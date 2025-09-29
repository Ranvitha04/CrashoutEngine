#pragma once
#include "math/CEVec3.h"

namespace Component {

    enum class ShapeType { Square, Rectangle, Line, Triangle };

    struct ShapeComponent {
        bool enableTexture{ false };

        int wrapModeU{ 0 };
        int wrapModeV{ 0 };
        CEVec2 uvSize{ 1.f, 1.f };
        std::string textureName{};

        ShapeType type{ ShapeType::Rectangle };

        // shared
        CEVec3 color{ 255.f,255.f,255.f };
        // float rotation{ 0.f };
        // float scale{ 1.f };

        // square
        float size{ 32.f };

        // rectangle
        float width{ 32.f }, height{ 32.f };

        // line
        CEVec3 p1{ 0.f,0.f,0.f };
        CEVec3 p2{ 32.f,0.f,0.f };
        float thickness{ 1.f };

        // triangle
        CEVec3 t1{ -16.f,-16.f,0.f };
        CEVec3 t2{ 16.f,-16.f,0.f };
        CEVec3 t3{ 0.f,16.f,0.f };

        // toggle
        bool preview = false;
    };

} // namespace Component