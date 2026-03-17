/*!
@file       shapescomponent.h
@author     Renfred Lee
@author     Renfred (RTIS)
@author     Ranvitha
@date       28/09/2025
@brief	    This file defines the ShapesComponent, a simple data structure
            used to store an entity's shape type and variables.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec3.h"
#include "CEColor.h"
#include <GL/glew.h>

/*
----------------------------------------------------------------------------- */
namespace Component {

    /**
    * @enum ShapeType
    * @brief Enumerates the primitive shape represented by a ShapeComponent.
    *
    * @var ShapeType::Rectangle
    *   An axis-aligned rectangle described by width/height.
    * @var ShapeType::Line
    *   A line segment described by two endpoints and a thickness.
    * @var ShapeType::Triangle
    *   A triangle described by three vertices.
    */
    enum class ShapeType { Rectangle, Line, Triangle, Circle };

    /**
     * @struct ShapeComponent
     * @brief Data describing a simple drawable primitive with optional texture.
     *
     * @details
     * Fields are grouped into:
     * - Shared fields (apply to all shapes).
     * - Shape-specific fields (used only for the corresponding ShapeType).
     * - Debug/preview toggles.
     */
    struct ShapeComponent {
        // -------------------- TextureAsset Settings --------------------

        bool enableTexture{ false }; ///< Toggle to enable texturing for this shape.
        bool isWorldSpace{ true }; ///< Toggle to enable world space / view space

        int  wrapModeU{ 0 }; ///< U wrap mode (renderer-defined enum/integer).
        int  wrapModeV{ 0 }; ///< V wrap mode (renderer-defined enum/integer).

        CEVec2 uvSize{ 1.f, 1.f }; ///< UV tiling/repeat size when texturing is enabled.
        CEVec2 uvOffset{ 0.f, 0.f };
        std::string textureName{ "missing.png" }; ///< Name/identifier of the texture resource to sample.
        GLuint textureHandle{ 0 }; ///< Optional raw handle override (e.g. for VideoComponent)

        // -------------------- Shape Selection --------------------

        ShapeType type{ ShapeType::Rectangle }; ///< Which primitive this component represents.

        // -------------------- Shared (All Shapes) -----------------

        CEColor color{ 1.f, 1.f, 1.f, 1.f }; ///< RGB color

        // -------------------- Square ------------------------------

        //float size{ 32.f }; ///< Edge length for @ref ShapeType::Square.

        // -------------------- Rectangle --------------------------

        //float width{ 32.f }, ///< Width for @ref ShapeType::Rectangle.
        //      height{ 32.f };///< Height for @ref ShapeType::Rectangle.

        // -------------------- Line -------------------------------

        CEVec3 p1{ 0.f,  0.f, 0.f }; ///< Line start point for @ref ShapeType::Line.
        CEVec3 p2{ 32.f, 0.f, 0.f }; ///< Line end point for @ref ShapeType::Line.
        float  thickness{ 1.f }; ///< Pixel/world thickness for lines.

        // -------------------- Triangle ---------------------------

        //CEVec3 t1{ -16.f, -16.f, 0.f }; ///< Vertex 1 for @ref ShapeType::Triangle.
        //CEVec3 t2{ 16.f, -16.f, 0.f }; ///< Vertex 2 for @ref ShapeType::Triangle.
        //CEVec3 t3{ 0.f,  16.f, 0.f }; ///< Vertex 3 for @ref ShapeType::Triangle.

        // -------------------- Debug / Preview --------------------

        bool preview{ false }; ///< If true, systems may render an editor preview.
    };

} // namespace Component