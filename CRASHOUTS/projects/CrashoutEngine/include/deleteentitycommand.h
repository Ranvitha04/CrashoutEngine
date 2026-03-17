/*!
@file       deleteentitycommand.h
@author     Shahrin Idzuan
@author     Afiq
@date       13/01/2026
@brief      [Brief description of deleteentitycommand.h]

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#pragma once
#include "undomanager.h"
#include "sceneasset.h"
#include "entitymanager.h"
#include <string>
#include <vector>

namespace CrashoutsEngine {

    class DeleteEntityCommand : public EditorCommand {
    public:
        DeleteEntityCommand(Entity entityId);
        ~DeleteEntityCommand() override = default;

        void Execute() override; // Performs the deletion
        void Undo() override;    // Restores the entity hierarchy

    private:
        Entity m_targetEntity;
        uint64_t m_originalParentUUID{};
        std::string m_jsonHierarchy; // Stores the serialized hierarchy as a JSON array string
    };

}

