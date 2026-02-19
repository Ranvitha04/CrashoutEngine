/*!
@file       deleteentitycommand.cpp
@author     m.biniswandi@digipen.edu
@date       13/01/2026
@brief      Implementation of DeleteEntityCommand for entity deletion with undo/redo functionality.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#include "deleteentitycommand.h"
#include "sceneasset.h"
#include "entitymanager.h"
#include "hierarchycomponent.h"
#include "uuidcomponent.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <iostream>

namespace CrashoutsEngine {

    DeleteEntityCommand::DeleteEntityCommand(Entity entityId) : m_targetEntity(entityId) {
        ECS& ecs = *ECS::getInstance();

        // Store Parent ID to re-attach later
        if (ecs.HasComponent<Component::ParentComponent>(entityId)) {
            m_originalParentId = ecs.GetComponent<Component::ParentComponent>(entityId)->parent;
        }

        // Serialize Hierarchy (Root + Descendants)
        rapidjson::Document doc;
        doc.SetArray();
        auto& allocator = doc.GetAllocator();

        std::vector<Entity> queue;
        queue.push_back(entityId);

        size_t head = 0;
        while (head < queue.size()) {
            Entity curr = queue[head++];
            
            // Serialize current entity
            rapidjson::Value val = Serialization::SerializeEntity(curr, allocator);
            doc.PushBack(val, allocator);

            // Add children to queue
            if (ecs.HasComponent<Component::ChildrenComponent>(curr)) {
                auto* childrenComp = ecs.GetComponent<Component::ChildrenComponent>(curr);
                for (Entity child : childrenComp->children) {
                    queue.push_back(child);
                }
            }
        }

        // Convert to JSON String
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        m_jsonHierarchy = buffer.GetString();
    }

    void DeleteEntityCommand::Execute() {
        // "Execute" performs the deletion
        ECS& ecs = *ECS::getInstance();
        ecs.DestroyEntity(m_targetEntity);
    }

    void DeleteEntityCommand::Undo() {
        ECS& ecs = *ECS::getInstance();
        rapidjson::Document doc;
        doc.Parse(m_jsonHierarchy.c_str());

        if (doc.HasParseError() || !doc.IsArray()) {
            std::cerr << "DeleteEntityCommand::Undo - Failed to parse JSON hierarchy." << std::endl;
            return;
        }

        std::vector<Serialization::ParentLinkInfo> links;
        std::vector<Entity> restoredEntities;

        // 1. Deserialize All Entities
        for (const auto& val : doc.GetArray()) {
            Entity newId = static_cast<Entity>(Serialization::DeserializeEntity(val, &links));
            restoredEntities.push_back(newId);
        }

        if (restoredEntities.empty()) return;

        // The first entity serialized was the root.
        Entity newRootId = restoredEntities[0];
        m_targetEntity = newRootId; // Update for next Redo

        // 2. Process Parent Links
        // We need to link entities to their parents.
        // The parents could be:
        // A) Inside the restored group (internal hierarchy)
        // B) Outside the restored group (the original parent of the root)

        // Optimization: Map Name -> NewEntityID for the restored group
        std::unordered_map<uint64_t, Entity> restoredMap;
        for (Entity e : restoredEntities) {
            if (ecs.HasComponent<Component::UUIDComponent>(e)) {
                restoredMap[ecs.GetComponent<Component::UUIDComponent>(e)->id] = e;
            }
        }

        for (const auto& req : links) {
            Entity child = req.childEntity;
            Entity parent = 0;

            // Try to find parent in restored group first
            auto it = restoredMap.find(req.parentUUID);
            if (it != restoredMap.end()) {
                parent = it->second;
            }
            // Else look globally (for external parent)
            else {
                parent = ecs.FindEntityByUUID(req.parentUUID);
            }

            if (parent != 0) {
                // Link
                if (ecs.HasComponent<Component::ParentComponent>(child)) {
                    ecs.GetComponent<Component::ParentComponent>(child)->parent = parent;
                }
                if (!ecs.HasComponent<Component::ChildrenComponent>(parent)) {
                    ecs.AddComponent<Component::ChildrenComponent>(parent);
                }
                ecs.GetComponent<Component::ChildrenComponent>(parent)->children.push_back(child);
            }
        }

        // Force re-attach root to original parent ID if valid
        if (m_originalParentId != 0) {
            // Check if parent still exists has UUIDComponent is a good proxy for 'alive'
            if (ecs.HasComponent<Component::UUIDComponent>(m_originalParentId)) {
                
                // Fix ParentComponent on Child
                if (!ecs.HasComponent<Component::ParentComponent>(newRootId)) {
                    ecs.AddComponent<Component::ParentComponent>(newRootId);
                }
                ecs.GetComponent<Component::ParentComponent>(newRootId)->parent = m_originalParentId;

                // Fix ChildrenComponent on Parent
                if (!ecs.HasComponent<Component::ChildrenComponent>(m_originalParentId)) {
                    ecs.AddComponent<Component::ChildrenComponent>(m_originalParentId);
                }
                auto* children = ecs.GetComponent<Component::ChildrenComponent>(m_originalParentId);
                
                // Avoid duplicates just in case
                bool exists = false;
                for (Entity c : children->children) { if (c == newRootId) exists = true; }
                if (!exists) children->children.push_back(newRootId);
            }
        }
    }
}

