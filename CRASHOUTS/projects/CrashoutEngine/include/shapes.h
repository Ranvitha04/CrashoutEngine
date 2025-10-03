#ifndef SHAPES_H
#define SHAPES_H

/*!
@file        shapes.h
@author      ranvitha@digipen.edu
@date        05/05/2025

This file declares the Shapes struct, which provides static utility
functions to create and draw basic geometric primitives such as lines,
squares, rectangles, and triangles. Each shape function encapsulates
OpenGL buffer creation, attribute binding, and rendering configuration.
Optional parameters allow transformations such as rotation, scaling,
and uniform coloring.

The Shapes struct also provides a cleanup function to release
rendering resources associated with these primitives.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "math/CEVec3.h"
#include "math/CEVec2.h"
#include <iostream>
                         
namespace Component {

    /*!
    @struct Shapes
    @brief Static utility struct for creating and rendering geometric primitives.
    @details
    Provides a collection of static functions for creating and drawing basic 2D shapes
    including squares, rectangles, lines, triangles, and circles. Supports both solid
    color and textured rendering with transformation capabilities (position, rotation, scale).
    Also includes instanced rendering for efficient batch drawing of multiple objects.
    */
    struct Shapes {

        /*!
        @brief Create and render a square primitive.
        @param position World position for the square center
        @param size Side length of the square (default: 1.0f)
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param color RGB color values (default: white)
        @details
        Creates a square centered at the specified position with configurable size,
        rotation, and color. Uses triangle fan rendering for optimal performance.
        */
        static void CreateSquare(const CEVec3& position, float size = 1.0f, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(255.0f, 255.0f, 255.0f));

        /*!
        @brief Create and render a line primitive.
        @param start Starting point of the line
        @param end Ending point of the line
        @param thickness Line thickness in world units (default: 1.0f)
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param color RGB color values (default: white)
        @details
        Creates a line segment between two points with configurable thickness.
        The line is rendered as a rectangle oriented along the start-end vector.
        */
        static void CreateLine(const CEVec3& start, const CEVec3& end, float thickness = 1.0f, float rotation = 0.0f, float scale = 1.0f,const CEVec3& color = CEVec3(255.0f, 255.0f, 255.0f));

        /*!
        @brief Create and render a rectangle primitive.
        @param position World position for the rectangle center
        @param width Rectangle width in world units
        @param height Rectangle height in world units
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param color RGB color values (default: white)
        @details
        Creates a rectangle centered at the specified position with configurable
        dimensions, rotation, and color. Uses triangle fan rendering.
        */
        static void CreateRectangle(const CEVec3& position, float width, float height, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(255.0f, 255.0f, 255.0f));

        /*!
        @brief Create and render a triangle primitive.
        @param p1 First vertex position
        @param p2 Second vertex position
        @param p3 Third vertex position
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param color RGB color values (default: white)
        @details
        Creates a triangle using three specified vertices with configurable
        rotation and color. The triangle is rendered using triangle primitive.
        */
        static void CreateTriangle(const CEVec3& p1, const CEVec3& p2, const CEVec3& p3, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(255.0f, 255.0f, 255.0f));
        
        /*!
        @brief Clean up all shape rendering resources.
        @details
        Releases all OpenGL resources (VAOs, VBOs, EBOs) used by the shapes
        rendering system. Should be called during application shutdown.
        */
        static void Cleanup();

        /*!
        @brief Create and render a textured square primitive.
        @param textureName Name of the texture asset to apply
        @param position World position for the square center
        @param size Side length of the square
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param tint Color tint to apply to the texture (default: white)
        @param uvSize UV coordinate scaling for texture mapping (default: 1.0f, 1.0f)
        @details
        Creates a textured square using the specified texture asset. The texture
        is loaded from the AssetManager and applied with optional color tinting.
        */
        static void CreateTexturedSquare(const std::string& textureName, const CEVec3& position, float size,
            float rotation = 0.0f, float scale = 1.0f, const CEVec3& tint = CEVec3(255.0f, 255.0f, 255.0f),
            CEVec2 const& uvSize = CEVec2(1.f, 1.f));

        /*!
        @brief Create and render a textured rectangle primitive.
        @param textureName Name of the texture asset to apply
        @param position World position for the rectangle center
        @param width Rectangle width in world units
        @param height Rectangle height in world units
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param tint Color tint to apply to the texture (default: white)
        @param uvSize UV coordinate scaling for texture mapping (default: 1.0f, 1.0f)
        @details
        Creates a textured rectangle using the specified texture asset with
        configurable dimensions and optional color tinting.
        */
        static void CreateTexturedRectangle(const std::string& textureName, const CEVec3& position,
            float width, float height, float rotation = 0.0f,
            float scale = 1.0f, const CEVec3& tint = CEVec3(255.0f, 255.0f, 255.0f),
            CEVec2 const& uvSize = CEVec2(1.f, 1.f));

        /*!
        @brief Create and render a textured triangle primitive.
        @param textureName Name of the texture asset to apply
        @param p1 First vertex position
        @param p2 Second vertex position
        @param p3 Third vertex position
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param tint Color tint to apply to the texture (default: white)
        @param uvSize UV coordinate scaling for texture mapping (default: 1.0f, 1.0f)
        @details
        Creates a textured triangle using three specified vertices and the
        specified texture asset with optional color tinting.
        */
        static void CreateTexturedTriangle(const std::string& textureName, const CEVec3& p1, const CEVec3& p2,
            const CEVec3& p3, float rotation = 0.0f, float scale = 1.0f,
            const CEVec3& tint = CEVec3(255.0f, 255.0f, 255.0f),
            CEVec2 const& uvSize = CEVec2(1.f, 1.f));

        /*!
        @brief Create and render a circle primitive.
        @param position World position for the circle center
        @param radius Circle radius in world units (default: 1.0f)
        @param segments Number of segments for circle approximation (default: 32)
        @param rotation Rotation angle in degrees (default: 0.0f)
        @param scale Uniform scaling factor (default: 1.0f)
        @param color RGB color values (default: white)
        @details
        Creates a circle approximated using a triangle fan with the specified
        number of segments. Higher segment counts produce smoother circles.
        */
        static void CreateCircle(const CEVec3& position, float radius = 1.0f, int segments = 32, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(1.0f, 1.0f, 1.0f));

        /*!
        @brief Create and render multiple square instances efficiently.
        @param positions Array of world positions for each square
        @param colors Array of colors for each square
        @param rotations Array of rotation angles for each square
        @param scales Array of scale factors for each square
        @param count Number of squares to render
        @param baseSize Base size for all squares (default: 1.0f)
        @details
        Efficiently renders multiple squares using instanced rendering techniques.
        All arrays must contain at least 'count' elements. This method is
        optimized for rendering large numbers of similar objects.
        */
        static void CreateInstancedSquares(const CEVec3* positions, const CEVec3* colors, 
                                         const float* rotations, const float* scales, 
                                         int count, float baseSize = 1.0f);

    };

    



}

#endif // SHAPES_H