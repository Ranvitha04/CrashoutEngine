#pragma once

#include "undomanager.h"
#include "entitymanager.h"
#include <string>

namespace CrashoutsEngine {

    class CreateEntityCommand : public EditorCommand {
    public:
        CreateEntityCommand();
        ~CreateEntityCommand() override = default;

        void Execute() override; // Create (or recreate on redo)
        void Undo() override;    // Destroy

        Entity GetCreatedEntity() const { return m_createdEntity; }

    private:
        Entity m_createdEntity = 0;
        std::string m_entityJson; // snapshot of the created entity
    };

}