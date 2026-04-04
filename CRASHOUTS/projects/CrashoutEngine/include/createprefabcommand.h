/*!
@file       createprefabcommand.h
@author     Afiq
@date       13/03/2026
@brief      Declaration of an editor command for creating a prefab instance with undo support.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once

#include "undomanager.h"
#include <string>

namespace CrashoutsEngine {

    /*!
    @class CreatePrefabCommand
    @brief Instantiates a prefab (optionally at a position) and supports undo by destroying it.
    */
    /** @brief CreatePrefabCommand */
    class CreatePrefabCommand : public EditorCommand {
    public:
        /*!
        @brief Construct a command to create a prefab using its default transform.
        @param prefabName Name/identifier of the prefab to instantiate.
        */
        explicit CreatePrefabCommand(const std::string& prefabName);

        /*!
        @brief Construct a command to create a prefab at a given world position.
        @param prefabName Name/identifier of the prefab to instantiate.
        @param x X position.
        @param y Y position.
        @param z Z position.
        */
        CreatePrefabCommand(const std::string& prefabName, float x, float y, float z);

        /*!
        @brief Execute the command by instantiating the prefab.
        */
        void Execute() override;

        /*!
        @brief Undo the command by destroying the instantiated prefab root entity.
        */
        void Undo() override;

    private:
        std::string m_prefabName;
        Entity m_createdRoot = 0;

        bool m_usePosition = false;
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

}
