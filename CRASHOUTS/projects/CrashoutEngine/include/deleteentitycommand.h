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
        Entity m_originalParentId = 0;
        std::string m_jsonHierarchy; // Stores the serialized hierarchy as a JSON array string
    };

}
