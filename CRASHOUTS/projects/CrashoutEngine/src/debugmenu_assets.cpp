/*!
@file        debugmenu_assets.cpp
@author      Mohamad Amir Akmal
@author      Renfred Lee
@author      Shahrin Idzuan
@date        25/12/2025
@brief       File containing the implementation of the Asset Browser panel in the debug menu.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#include "debugmenu_internals.h"
#include "engine.h"

namespace UI {

    std::vector<std::string> gDroppedPaths;

    /*!
     @brief Draw an asset browser panel with folder navigation, filtering, drag-and-drop, and asset reloading.
     @param ui UI state reference.
     @param root Root path for assets.
     @param outSelectedAsset Output parameter for the selected asset path.
    */
    void RenderAssetBrowserPanel(UIState& ui, std::string& outSelectedAsset)
    {
        ScopedWindow win("Assets", LockFlags(ui.lockLayout));

        namespace fs = std::filesystem;

        static char filter[128] = { 0 };
        static bool s_showUnsupportedPopup = false;
        static std::string s_lastErrorFile = "";

        // State for deletion popup
        static std::string fileToDeletePath = "";
        static std::string fileToDeleteName = "";
        static bool openDeletePopup = false;

        // panel mode � 0 = normal assets, 1 = C# scripts
        static int s_location = 0;
        ImGui::Combo("Location", &s_location, "Assets\0Scripts\0");

        fs::path rootPath;
        if (s_location == 0)
            rootPath = fs::path(GetGameRoot()) / "assets";
        else
            rootPath = fs::path(GetGameRoot()) / "scripts";

        static fs::path current;
        static fs::path lastRoot;
        static std::vector<fs::directory_entry> entries;
        static bool needScan = true;

        if (current.empty() || lastRoot != rootPath) {
            current = rootPath;
            lastRoot = rootPath;
            needScan = true;
        }

        {
            std::string fname = current.filename().string();
            if (fname == "obj" || fname == "bin" || fname == ".vs" || fname == ".idea") {
                current = rootPath;
                needScan = true;
            }
        }

        // Top bar controls
        if (ImGui::Button("Up") && current.has_parent_path()) {
            current = current.parent_path();
            needScan = true;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(current.generic_string().c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##asset_filter", "Filter (name / ext)...", filter, IM_ARRAYSIZE(filter));

        ImGui::SameLine();
        // ImGUI button to reload assets
        if (ImGui::Button("Reload Assets")) {
            using namespace CrashoutsEngine::Assets;
            AssetManager::GetInstance().LoadManifestAssets();
        }

        ImGui::Separator();

        if (!gDroppedPaths.empty()) {
            namespace fs = std::filesystem;

            for (const std::string& absPath : gDroppedPaths) {
                std::error_code ec;
                fs::path src(absPath);
                if (!fs::is_regular_file(src, ec)) continue;

                std::string ext = src.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                bool allowed =
                    ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" ||
                    ext == ".json" || ext == ".scene" || ext == ".bank" || ext == ".wav" || ext == ".ogg" || ext == ".mp3" ||
                    ext == ".vert" || ext == ".frag" || ext == ".cs";

                if (!allowed) {
                    s_showUnsupportedPopup = true;
                    s_lastErrorFile = src.filename().string();
                    ImGui::OpenPopup("Asset Error");
                    continue;
                }

                fs::path dst = current / src.filename();
                if (fs::exists(dst, ec)) {
                    for (int i = 1; i < 10000; ++i) {
                        fs::path candidate =
                            dst.parent_path() /
                            (dst.stem().string() + " (" + std::to_string(i) + ")" + dst.extension().string());
                        if (!fs::exists(candidate, ec)) { dst = candidate; break; }
                    }
                }

                fs::create_directories(dst.parent_path(), ec);
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
                CrashoutsEngine::Assets::AssetManager::GetInstance().LoadSingleAsset(dst.generic_string());
            }

            gDroppedPaths.clear();
            needScan = true;
        }

        if (ImGui::BeginPopupModal("Asset Error", &s_showUnsupportedPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "Failed to import file:");
            ImGui::Text("%s", s_lastErrorFile.c_str());
            ImGui::Separator();
            ImGui::Text("Unsupported file format.");
            ImGui::TextDisabled("Accepted: images (.png, .jpg...), audio (.wav, .ogg, .bank), scenes, scripts (.cs)");

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                s_showUnsupportedPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // LEFT: Folder tree
        ImGui::BeginChild("asset_left", ImVec2(100, 0), true);

        try {
            for (auto& de : fs::directory_iterator(rootPath)) {
                if (!de.is_directory()) continue;

                std::string fname = de.path().filename().string();
                if (fname == "obj" || fname == "bin" || fname == ".vs" || fname == ".idea") continue;

                bool isCur = (current == de.path());
                if (ImGui::Selectable(de.path().filename().generic_string().c_str(), isCur)) {
                    current = de.path();
                    needScan = true;
                }
            }
        }
        catch (...) {}

        fs::path scriptsRoot = fs::path(GetGameRoot()) / "scripts";
        bool isScripts = (current == scriptsRoot);
        if (ImGui::Selectable("scripts", isScripts)) {
            current = scriptsRoot;
            needScan = true;
        }

        ImGui::EndChild();

        // RIGHT: Files + Preview
        ImGui::SameLine();
        ImVec2 rightArea = ImGui::GetContentRegionAvail();

        float listWidth = 200.0f;

        // FILE LIST 
        ImGui::BeginChild("asset_right_list", ImVec2(listWidth, 0), true);

        if (needScan) {
            entries.clear();
            try {
                for (auto& e : fs::directory_iterator(current))
                    entries.push_back(e);

                std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b) {
                    if (a.is_directory() != b.is_directory()) return a.is_directory() && !b.is_directory();
                    return a.path().filename().generic_string() < b.path().filename().generic_string();
                    });
            }
            catch (...) {}
            needScan = false;
        }

        // Build filtered list of indices
        static std::vector<int> vis;
        vis.clear();
        vis.reserve(entries.size());

        for (int i = 0; i < (int)entries.size(); ++i) {
            const auto& e = entries[i];
            std::string name = e.path().filename().generic_string();

            if (name == "obj" || name == "bin" || name == ".vs" || name == ".idea")
                continue;

            if (filter[0] && name.find(filter) == std::string::npos)
                continue;

            vis.push_back(i);
        }

        ImGuiListClipper clip;
        clip.Begin((int)vis.size());
        while (clip.Step()) {
            for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row) {
                const auto& e = entries[vis[row]];
                std::string name = e.path().filename().generic_string();

                if (e.is_directory()) {
                    if (ImGui::Selectable((std::string("[Dir] ") + name).c_str(), false)) {
                        current = e.path(); needScan = true;
                    }
                }
                else {
                    bool sel = (outSelectedAsset == e.path().generic_string());

                    // --- THE FILE ITEM ---
                    if (ImGui::Selectable(name.c_str(), sel))
                        outSelectedAsset = e.path().generic_string();

                    // --- Context Menu for Deletion ---
                    std::string contextId = "FileContext_" + name + "_" + std::to_string(row);
                    if (ImGui::BeginPopupContextItem(contextId.c_str())) {
                        if (ImGui::MenuItem("Delete File")) {
                            fileToDeletePath = e.path().generic_string();
                            fileToDeleteName = name;
                            openDeletePopup = true; // Signal the modal to open outside the loop
                        }
                        ImGui::EndPopup();
                    }
                    // --------------------------------------

                    if (ImGui::IsItemHovered() &&
                        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        outSelectedAsset = e.path().generic_string();

                        std::string ext = e.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
                        {

                            std::string rel = fs::relative(e.path(), rootPath).generic_string();

                            SpriteSheet_SetTexturePath(rel.c_str());
                            ui.showSpriteSheet = true;
                        }

                        else if (ext == ".cs")
                        {
                            std::string cmd = "start \"\" \"" + e.path().string() + "\"";
                            system(cmd.c_str());
                        }
                    }

                    // Drag Drop Source Logic...
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        AssetDragPayload p{};
                        std::string rel = fs::relative(e.path(), rootPath).generic_string();
                        std::string stem = e.path().stem().generic_string();
                        std::string ext = e.path().extension().generic_string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        // Heuristics to pick kind by folder/extension
                        if (rel.rfind("images/", 0) == 0 ||
                            ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                            p.kind = AssetKind::Texture;
                        }
                        else if (rel.rfind("sprite_sheet/", 0) == 0) {
                            p.kind = AssetKind::SpriteSheet;
                        }
                        else if (rel.rfind("scenes/", 0) == 0 || ext == ".scene") {
                            p.kind = AssetKind::Scene;
                        }
                        else if (rel.rfind("audio/", 0) == 0 || ext == ".bank" || ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
                            p.kind = AssetKind::Audio;
                        }
                        else if (rel.rfind("prefabs/", 0) == 0) {
                            p.kind = AssetKind::Prefab;
                        }
                        else if (rel.rfind("shaders/", 0) == 0 || ext == ".vert" || ext == ".frag") {
                            p.kind = AssetKind::Shader;
                        }
                        else {
                            p.kind = AssetKind::Unknown;
                        }

                        std::strncpy(p.name, stem.c_str(), sizeof(p.name) - 1);
                        std::strncpy(p.relPath, rel.c_str(), sizeof(p.relPath) - 1);

                        ImGui::SetDragDropPayload("ASSET/PAYLOAD", &p, sizeof(p));
                        ImGui::TextUnformatted(p.name);
                        ImGui::EndDragDropSource();
                    }
                }
            }
        }
        ImGui::EndChild(); // asset_right_list

        // --- Safely draw the Delete Confirmation Modal outside the Clipper ---
        if (openDeletePopup) {
            ImGui::OpenPopup("Delete Confirmation");
            openDeletePopup = false; // Reset the signal
        }

        if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete this file?");
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", fileToDeleteName.c_str());
            ImGui::Separator();
            ImGui::TextDisabled("This action cannot be undone.");

            if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                try {
                    fs::remove(fileToDeletePath);
                    std::cout << "[AssetBrowser] Deleted: " << fileToDeletePath << std::endl;

                    // If we deleted the file we were currently previewing, clear the preview
                    if (outSelectedAsset == fileToDeletePath) {
                        outSelectedAsset = "";
                    }

                    needScan = true; // Force a refresh of the file list
                }
                catch (const fs::filesystem_error& err) {
                    std::cerr << "[AssetBrowser] Delete failed: " << err.what() << std::endl;
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        // ------------------------------------------------------------------------

        // PREVIEW 
        ImGui::SameLine();
        ImGui::BeginChild("asset_right_preview", ImVec2(0, 0), true);

        if (!outSelectedAsset.empty()) {
            std::string ext = fs::path(outSelectedAsset).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                static GLuint previewTex = 0;
                static std::string loadedPath;
                static int imgW = 0, imgH = 0;

                // Load texture when selection changes
                if (loadedPath != outSelectedAsset) {
                    if (previewTex) {
                        glDeleteTextures(1, &previewTex);
                        previewTex = 0;
                    }

                    int w, h, channels;
                    unsigned char* data = stbi_load(outSelectedAsset.c_str(), &w, &h, &channels, 4);
                    if (data) {
                        const int MAX_DIM = 4096;
                        if (w > MAX_DIM || h > MAX_DIM) {
                            std::cout << "FAILED: oversize preview (" << w << "x" << h << ") -> " << outSelectedAsset << std::endl;
                            stbi_image_free(data);
                            data = nullptr;
                        }
                    }
                    if (data) {
                        glGenTextures(1, &previewTex);
                        glBindTexture(GL_TEXTURE_2D, previewTex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        stbi_image_free(data);
                        loadedPath = outSelectedAsset;
                        imgW = w; imgH = h;
                    }
                }

                if (previewTex && imgW > 0 && imgH > 0) {
                    ImGui::TextUnformatted("Preview:");

                    const float kPreviewMaxHeight = 260.0f;
                    ImVec2 area = ImGui::GetContentRegionAvail();
                    area.y = std::min(area.y, kPreviewMaxHeight);

                    float maxW = area.x;
                    float maxH = area.y;
                    float scale = std::min(maxW / float(imgW), maxH / float(imgH));
                    if (scale <= 0.0f) scale = 1.0f;
                    ImVec2 drawSize(float(imgW) * scale, float(imgH) * scale);

                    float xPad = (area.x - drawSize.x) * 0.5f;
                    if (xPad > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPad);

                    ImGui::Image((ImTextureID)(intptr_t)previewTex, drawSize);
                }
            }

            if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
            {
                ImGui::TextUnformatted("Audio File:");
                ImGui::Text("%s", fs::path(outSelectedAsset).filename().string().c_str());

                static std::string lastPreview = "";
                static bool previewPlaying = false;

                if (ImGui::Button(previewPlaying ? "Stop" : "Play"))
                {
                    if (!previewPlaying)
                    {
                        CrashoutsEngine::Audio::AudioSystem::instance().playSFXFile(outSelectedAsset, false, 1.0f, 100.0f);

                        previewPlaying = true;
                        lastPreview = outSelectedAsset;
                    }
                    else
                    {
                        CrashoutsEngine::Audio::AudioSystem::instance().stopSFXGroup();
                        previewPlaying = false;
                    }
                }

                // Auto-stop if the user selects a new asset
                if (lastPreview != outSelectedAsset && previewPlaying)
                {
                    CrashoutsEngine::Audio::AudioSystem::instance().stopSFXGroup();
                    previewPlaying = false;
                }
            }

            if (ext == ".cs")
            {
                ImGui::Separator();
                ImGui::TextUnformatted("C# Script:");
                ImGui::Text("%s", fs::path(outSelectedAsset).filename().string().c_str());

                if (ImGui::Button("Open in C# editor"))
                {
                    std::string cmd = "start \"\" \"" + outSelectedAsset + "\"";
                    system(cmd.c_str());
                }
            }
        }

        ImGui::EndChild(); // asset_right_preview
    }
}