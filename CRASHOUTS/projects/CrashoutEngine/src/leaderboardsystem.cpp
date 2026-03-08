/*
@file       leaderboardsystem.cpp
@author
@date       04/03/2026
@brief      Runtime update system for LeaderboardComponent.
*/
#include "leaderboardsystem.h"

#include "leaderboardcomponent.h"
#include "LeaderboardService.h"
#include "entitymanager.h"
#include "namecomponent.h"
#include "textcomponent.h"
#include "hierarchy.h"
#include "activecomponent.h"

#include <unordered_map>
#include <future>
#include <algorithm>
#include <chrono>

namespace {
    using CrashoutsEngine::Entity;

    struct PendingFetch {
        std::future<std::vector<CrashoutsEngine::LeaderboardEntry>> future;
    };

    static std::unordered_map<Entity, PendingFetch> g_pendingFetches;

    static void ReplaceAll(std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t start = 0;
        while ((start = s.find(from, start)) != std::string::npos) {
            s.replace(start, from.length(), to);
            start += to.length();
        }
    }

    static std::string GetFieldValue(const Component::LeaderboardEntryData& e, const std::string& key) {
        if (key == "rank") return std::to_string(e.rank);
        if (key == "name") return e.name;
        if (key == "score") return std::to_string(e.score);
        if (key == "created_at") return e.created_at;
        auto it = e.extraFields.find(key);
        if (it != e.extraFields.end()) return it->second;
        return "";
    }

    static std::string FormatScoreAsTime(int totalMs) {
        int mins = totalMs / 60000;
        int secs = (totalMs % 60000) / 1000;
        int ms = (totalMs % 1000) / 10;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d", mins, secs, ms);
        return std::string(buf);
    }

    static std::string FormatLine(const Component::LeaderboardComponent& comp,
        const Component::LeaderboardEntryData& e) {
        std::string out = comp.lineFormat;
        ReplaceAll(out, "{rank}", std::to_string(e.rank));
        ReplaceAll(out, "{name}", e.name);
        
        //ReplaceAll(out, "{score}", std::to_string(e.score));
        
        // For forcing the time format
        ReplaceAll(out, "{score}", FormatScoreAsTime(e.score));

        ReplaceAll(out, "{created_at}", e.created_at);
        for (const auto& kv : e.extraFields) {
            ReplaceAll(out, "{" + kv.first + "}", kv.second);
        }
        return out;
    }

    static CrashoutsEngine::Entity FindEntityByName(CrashoutsEngine::ECS& ecs, const std::string& name) {
        if (name.empty()) return 0;
        auto& arr = ecs.GetComponentArray<Component::NameComponent>();
        for (size_t i = 0; i < arr.dense.size(); ++i) {
            if (arr.dense[i]->name == name) return arr.indexToEntity[i];
        }
        return 0;
    }

    static void BuildNameMap(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity root,
        std::unordered_map<std::string, CrashoutsEngine::Entity>& out) {
        out.clear();
        if (!root) return;

        std::vector<CrashoutsEngine::Entity> descendants;
        descendants.reserve(64);
        CrashoutsEngine::GatherDescendants(ecs, root, descendants);
        descendants.push_back(root);

        for (CrashoutsEngine::Entity e : descendants) {
            if (!ecs.HasComponent<Component::NameComponent>(e)) continue;
            if (!ecs.HasComponent<Component::TextComponent>(e)) continue;
            const std::string& n = ecs.GetComponent<Component::NameComponent>(e)->name;
            out[n] = e;
        }
    }

    static void ApplyTextIfFound(CrashoutsEngine::ECS& ecs,
        const std::unordered_map<std::string, CrashoutsEngine::Entity>& nameMap,
        const std::string& name, const std::string& text) {
        if (name.empty()) return;
        auto it = nameMap.find(name);
        if (it == nameMap.end()) return;
        auto* tc = ecs.GetComponent<Component::TextComponent>(it->second);
        if (!tc) return;
        tc->text = text;
    }
}

namespace CrashoutsEngine::LeaderboardSystem {

    void Update(ECS& ecs, float dt, bool inEditor) {
        auto& arr = ecs.GetComponentArray<Component::LeaderboardComponent>();
        std::unordered_map<std::string, Entity> nameMap;

        std::unordered_map<Entity, bool> live;
        live.reserve(arr.indexToEntity.size());

        for (size_t i = 0; i < arr.dense.size(); ++i) {
            Component::LeaderboardComponent* comp = arr.dense[i];
            Entity entity = arr.indexToEntity[i];
            live[entity] = true;

            if (!comp) continue;
            if (inEditor && !comp->updateInEditor) continue;

            if (ecs.HasComponent<Component::ActiveComponent>(entity)) {
                if (!CrashoutsEngine::IsEntityActive(ecs, entity)) continue;
            }

            comp->refreshTimer += dt;

            bool shouldFetch = false;
            if (comp->requestNow) {
                shouldFetch = true;
                comp->requestNow = false;
            } else if (!comp->hasEverFetched && comp->fetchOnStart) {
                shouldFetch = true;
            } else if (comp->autoRefresh && comp->refreshIntervalSec > 0.0f &&
                comp->refreshTimer >= comp->refreshIntervalSec) {
                shouldFetch = true;
            }

            if (shouldFetch) {
                if (g_pendingFetches.find(entity) == g_pendingFetches.end()) {
                    LeaderboardService svc(comp->baseUrl);
                    g_pendingFetches[entity] = PendingFetch{ svc.FetchLeaderboardAsync(comp->limit) };
                    comp->refreshTimer = 0.0f;
                }
            }

            auto it = g_pendingFetches.find(entity);
            bool loading = false;
            if (it != g_pendingFetches.end()) {
                auto status = it->second.future.wait_for(std::chrono::seconds(0));
                if (status == std::future_status::ready) {
                    std::vector<CrashoutsEngine::LeaderboardEntry> results = it->second.future.get();
                    g_pendingFetches.erase(it);

                    comp->entries.clear();
                    comp->entries.reserve(results.size());

                    for (const auto& r : results) {
                        Component::LeaderboardEntryData e;
                        e.rank = r.rank;
                        e.name = r.name;
                        e.score = r.score;
                        e.created_at = r.created_at;
                        e.extraFields = r.extraFields;
                        comp->entries.push_back(std::move(e));
                    }

                    comp->hasEverFetched = true;
                    comp->lastError.clear();
                } else {
                    loading = true;
                }
            }

            // Push results into UI
            Entity root = entity;
            if (!comp->rootEntityName.empty()) {
                Entity namedRoot = FindEntityByName(ecs, comp->rootEntityName);
                if (namedRoot != 0) root = namedRoot;
            }

            BuildNameMap(ecs, root, nameMap);

            // Header + Status
            if (!comp->headerEntityName.empty() && !comp->headerText.empty()) {
                ApplyTextIfFound(ecs, nameMap, comp->headerEntityName, comp->headerText);
            }

            if (!comp->statusEntityName.empty()) {
                if (loading) {
                    ApplyTextIfFound(ecs, nameMap, comp->statusEntityName, comp->loadingText);
                } else if (!comp->lastError.empty()) {
                    ApplyTextIfFound(ecs, nameMap, comp->statusEntityName, comp->errorText);
                } else if (comp->entries.empty() && !comp->emptyStatusText.empty()) {
                    ApplyTextIfFound(ecs, nameMap, comp->statusEntityName, comp->emptyStatusText);
                } else {
                    ApplyTextIfFound(ecs, nameMap, comp->statusEntityName, "");
                }
            }

            // Entry lines / columns
            int maxLines = std::max(0, comp->limit);
            for (int lineIndex = 0; lineIndex < maxLines; ++lineIndex) {
                int displayIndex = comp->entryIndexBase + lineIndex;

                const bool hasEntry = lineIndex < (int)comp->entries.size();
                const Component::LeaderboardEntryData* entry = hasEntry ? &comp->entries[lineIndex] : nullptr;

                if (comp->useColumns) {
                    if (!comp->columnKeys.empty() && comp->columnKeys.size() == comp->columnPrefixes.size()) {
                        for (size_t c = 0; c < comp->columnKeys.size(); ++c) {
                            std::string targetName = comp->columnPrefixes[c] + std::to_string(displayIndex);
                            std::string text = comp->emptyLineText;
                            if (entry) text = GetFieldValue(*entry, comp->columnKeys[c]);
                            ApplyTextIfFound(ecs, nameMap, targetName, text);
                        }
                    } else {
                        std::string rankName = comp->rankNamePrefix + std::to_string(displayIndex);
                        std::string playerName = comp->playerNamePrefix + std::to_string(displayIndex);
                        std::string scoreName = comp->scoreNamePrefix + std::to_string(displayIndex);

                        std::string rankText = comp->emptyLineText;
                        std::string nameText = comp->emptyLineText;
                        std::string scoreText = comp->emptyLineText;
                        std::string dateText = comp->emptyLineText;

                        if (entry) {
                            rankText = std::to_string(entry->rank);
                            nameText = entry->name;
                            scoreText = std::to_string(entry->score);
                            dateText = entry->created_at;
                        }

                        ApplyTextIfFound(ecs, nameMap, rankName, rankText);
                        ApplyTextIfFound(ecs, nameMap, playerName, nameText);
                        ApplyTextIfFound(ecs, nameMap, scoreName, scoreText);

                        if (!comp->dateNamePrefix.empty()) {
                            std::string dateName = comp->dateNamePrefix + std::to_string(displayIndex);
                            ApplyTextIfFound(ecs, nameMap, dateName, dateText);
                        }
                    }
                } else {
                    std::string entryName = comp->entryNamePrefix + std::to_string(displayIndex);
                    std::string lineText = comp->emptyLineText;
                    if (entry) lineText = FormatLine(*comp, *entry);
                    ApplyTextIfFound(ecs, nameMap, entryName, lineText);
                }
            }

            // Render into own TextComponent (no prefab)
            if (comp->renderToSelfText && ecs.HasComponent<Component::TextComponent>(entity)) {
                std::string output;
                if (comp->selfTextIncludeHeader && !comp->headerText.empty()) {
                    output += comp->headerText;
                    output += "\n";
                }

                for (int lineIndex = 0; lineIndex < maxLines; ++lineIndex) {
                    if (lineIndex > 0) output += "\n";
                    if (lineIndex < (int)comp->entries.size()) {
                        if (comp->useColumns && !comp->columnKeys.empty() && comp->columnKeys.size() == comp->columnPrefixes.size()) {
                            const auto& e = comp->entries[lineIndex];
                            std::string row;
                            for (size_t c = 0; c < comp->columnKeys.size(); ++c) {
                                if (c > 0) row += comp->selfTextSeparator;
                                row += GetFieldValue(e, comp->columnKeys[c]);
                            }
                            output += row;
                        } else if (comp->useColumns) {
                            const auto& e = comp->entries[lineIndex];
                            std::string row = std::to_string(e.rank);
                            row += comp->selfTextSeparator + e.name;
                            row += comp->selfTextSeparator + std::to_string(e.score);
                            if (!e.created_at.empty()) row += comp->selfTextSeparator + e.created_at;
                            output += row;
                        } else {
                            output += FormatLine(*comp, comp->entries[lineIndex]);
                        }
                    } else {
                        output += comp->emptyLineText;
                    }
                }

                if (comp->selfTextIncludeStatus && !comp->statusEntityName.empty()) {
                    if (!output.empty()) output += "\n";
                    if (loading) output += comp->loadingText;
                    else if (!comp->lastError.empty()) output += comp->errorText;
                    else if (comp->entries.empty() && !comp->emptyStatusText.empty()) output += comp->emptyStatusText;
                }

                ecs.GetComponent<Component::TextComponent>(entity)->text = output;
            }
        }

        // Cleanup pending fetches for destroyed entities
        if (!g_pendingFetches.empty()) {
            for (auto it = g_pendingFetches.begin(); it != g_pendingFetches.end(); ) {
                if (live.find(it->first) == live.end()) it = g_pendingFetches.erase(it);
                else ++it;
            }
        }
    }

}
