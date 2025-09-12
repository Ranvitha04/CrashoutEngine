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



#include "rendering.h"
#include "entitymanager.h"
#include "CEVec3.h"
#include <array>
#include <GL/glew.h>

namespace Component {

    struct Shapes {

    // Function to create and draw a square
    static void CreateSquare(const CEVec3& position, float size = 1.0f, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(1.0f, 1.0f, 1.0f));

    // Function to create and draw a line
    static void CreateLine(const CEVec3& start, const CEVec3& end, float thickness = 1.0f, float rotation = 0.0f, float scale = 1.0f,const CEVec3& color = CEVec3(1.0f, 1.0f, 1.0f));

    // Function to create and draw a rectangle
    static void CreateRectangle(const CEVec3& position, float width, float height, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(1.0f, 1.0f, 1.0f));

    // Function to create and draw a triangle
    static void CreateTriangle(const CEVec3& p1, const CEVec3& p2, const CEVec3& p3, float rotation = 0.0f, float scale = 1.0f, const CEVec3& color = CEVec3(1.0f, 1.0f, 1.0f));
    
    // Function to clean up all shape rendering resources
    static void Cleanup();

    };

}

#endif // SHAPES_H