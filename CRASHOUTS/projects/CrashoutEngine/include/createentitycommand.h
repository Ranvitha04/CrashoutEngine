/*!
@file       createentitycommand.h
@author     Afiq
@date       13/03/2026
@brief      Declaration of an editor command for creating an entity with undo/redo support.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once

#include "undomanager.h"
#include "entitymanager.h"
#include <string>

namespace CrashoutsEngine {

    /*!
    @class CreateEntityCommand
    @brief Creates an entity and stores a snapshot so it can be recreated on redo.
    */
    /** @brief CreateEntityCommand */
    class CreateEntityCommand : public EditorCommand {
    public:
        /*!
        @brief Construct a create-entity command.
        */
        CreateEntityCommand();
        ~CreateEntityCommand() override = default;

        /*!
        @brief Execute the command by creating (or recreating) the entity.
        */
        void Execute() override;

        /*!
        @brief Undo the command by destroying the created entity.
        */
        void Undo() override;

        /*!
        @brief Get the entity created by the command.
        @return Entity id for the created entity, or 0 if none exists.
        */
        Entity GetCreatedEntity() const { return m_createdEntity; }

    private:
        Entity m_createdEntity = 0;
        std::string m_entityJson; // snapshot of the created entity
    };

}
