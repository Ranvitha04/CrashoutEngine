#pragma once
#include "math/CEVec3.h"

namespace Component {

    enum class ShapeType { Square, Rectangle, Line, Triangle };

    struct ShapeComponent {
        ShapeType type{ ShapeType::Square };

        // shared
        CEVec3 color{ 1.f,1.f,1.f };
        float rotation{ 0.f };
        float scale{ 1.f };

        // square
        float size{ 64.f };

        // rectangle
        float width{ 64.f }, height{ 32.f };

        // line
        CEVec3 p1{ 0.f,0.f,0.f };
        CEVec3 p2{ 64.f,0.f,0.f };
        float thickness{ 2.f };

        // triangle
        CEVec3 t1{ 0.f,0.f,0.f };
        CEVec3 t2{ 64.f,0.f,0.f };
        CEVec3 t3{ 32.f,32.f,0.f };

        // toggle
        bool preview = false;
    };

} // namespace Component