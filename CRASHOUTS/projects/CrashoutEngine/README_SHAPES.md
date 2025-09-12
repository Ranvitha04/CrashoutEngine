# Shapes Drawing Library

## Overview
This library provides simple functions to draw basic shapes in OpenGL using the existing rendering infrastructure of the CrashoutEngine. Each shape can be positioned, scaled, rotated, and colored as needed.

## Available Shapes
- Squares
- Lines
- Rectangles
- Triangles

## How to Use

### 1. Include the header
```cpp
#include "shapes.h"
```

### 2. Draw shapes
```cpp
// Draw a red square at position (0,0,0) with size 0.5, no rotation, and default scale
Shapes::CreateSquare(CEVec3(0.0f, 0.0f, 0.0f), 0.5f, 0.0f, 1.0f, CEVec3(1.0f, 0.0f, 0.0f));

// Draw a green line from (-0.5, 0.5, 0) to (0.5, -0.5, 0) with thickness 0.05 and no rotation
Shapes::CreateLine(CEVec3(-0.5f, 0.5f, 0.0f), CEVec3(0.5f, -0.5f, 0.0f), 0.05f, 0.0f, CEVec3(0.0f, 1.0f, 0.0f));

// Draw a blue rectangle at position (0,0,0) with width 0.8, height 0.3, 45-degree rotation, and default scale
Shapes::CreateRectangle(CEVec3(0.0f, 0.0f, 0.0f), 0.8f, 0.3f, 45.0f, 1.0f, CEVec3(0.0f, 0.0f, 1.0f));

// Draw a yellow triangle with 30-degree rotation and scale of 1.5
Shapes::CreateTriangle(
    CEVec3(0.0f, 0.5f, 0.0f),   // Top
    CEVec3(-0.5f, -0.5f, 0.0f),  // Bottom-left
    CEVec3(0.5f, -0.5f, 0.0f),   // Bottom-right
    30.0f,                       // Rotation in degrees
    1.5f,                        // Scale factor
    CEVec3(1.0f, 1.0f, 0.0f)     // Yellow color
);
```

## Function Parameters

### CreateSquare
- `position`: Center position of the square (CEVec3)
- `size`: Size of the square (float)
- `rotation`: Rotation in degrees around Z-axis (float)
- `scale`: Scale factor (float)
- `color`: Color of the square (CEVec3)

### CreateLine
- `start`: Starting point of the line (CEVec3)
- `end`: Ending point of the line (CEVec3)
- `thickness`: Thickness of the line (float)
- `rotation`: Additional rotation in degrees around the midpoint (float)
- `color`: Color of the line (CEVec3)

### CreateRectangle
- `position`: Center position of the rectangle (CEVec3)
- `width`: Width of the rectangle (float)
- `height`: Height of the rectangle (float)
- `rotation`: Rotation in degrees around Z-axis (float)
- `scale`: Scale factor (float)
- `color`: Color of the rectangle (CEVec3)

### CreateTriangle
- `p1`, `p2`, `p3`: The three vertices of the triangle (CEVec3)
- `rotation`: Rotation in degrees around the centroid (float)
- `scale`: Scale factor (float)
- `color`: Color of the triangle (CEVec3)

## Integration with Main Application

To use these shapes in your application, you can call the shape functions from your draw function:

```cpp
// In your draw function
void YourDrawFunction() {
    // Draw your shapes here
    Shapes::CreateSquare(CEVec3(0.0f, 0.0f, 0.0f), 0.5f, 0.0f, 1.0f, CEVec3(1.0f, 0.0f, 0.0f));
    Shapes::CreateLine(CEVec3(-0.5f, 0.5f, 0.0f), CEVec3(0.5f, -0.5f, 0.0f), 0.05f, 0.0f, CEVec3(0.0f, 1.0f, 0.0f));
}
```