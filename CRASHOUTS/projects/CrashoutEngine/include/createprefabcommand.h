#pragma once

#include "undomanager.h"
#include <string>

namespace CrashoutsEngine {

    class CreatePrefabCommand : public EditorCommand {
    public:
        explicit CreatePrefabCommand(const std::string& prefabName);
        CreatePrefabCommand(const std::string& prefabName, float x, float y, float z);

        void Execute() override;
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