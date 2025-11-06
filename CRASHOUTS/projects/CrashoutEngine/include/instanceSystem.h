#pragma once

/*!
@file        ranvitha.h
@author      ranvitha@digipen.edu
@date        11/06/2025
@brief

@brief
This source file contains the implementation of functions responsible for **instanced rendering**
of geometric shapes (circles, rectangles, and triangles) in the CrashoutsEngine framework.

The module leverages **OpenGL instancing** to efficiently render large batches of shapes,
each with its own transformation, color, and texture properties. It supports both textured
and non-textured variants, allowing flexible visual presentation and dynamic scene composition.

Key functionalities include:
- **Instanced shape creation**: Initializes GPU buffers for circles, rectangles, and triangles.
- **Batch rendering**: Renders multiple instances in a single draw call using shared geometry data.
- **ECS integration**: Initializes and populates entity data for ECS-based rendering workflows.
- **Texture and tint support**: Applies textures with adjustable UV scaling and color tinting.
- **Cleanup utilities**: Frees GPU buffers and resets instancing data when no longer needed.

This file works in conjunction with the `InstancedParams` structure and provides a unified API
for shape instancing that other systems (e.g., game states or rendering passes) can call to
generate procedurally varied, GPU-efficient visuals.
*/

#include "CEVec3.h"
#include "CEVec2.h"
#include "entitymanager.h"
#include "sceneutils.h"
#include "assetmanager.h"
#include "shapes.h"
#include "rendering.h"
#include <string>
#include <array>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "textureasset.h"


namespace InstancingSystem {

    struct InstancedParams {
        bool textured{ false };
        std::string textureName{};
        CEVec3 tint{ 1.f, 1.f, 1.f };
        CEVec2 uvSize{ 1.f, 1.f };
    };

    //....................Cirle................//

    void CreateInstancedCircles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments);

    void CreateInstancedCirclesTextured(const std::string& textureName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments,
        const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f),
        const CEVec2& uvSize = CEVec2(1.0f, 1.0f));

    void RenderInstancedCirclesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* radii,
        int count,
        int segments,
        const InstancedParams& params
    );        


    //...................Rectangle.................................//

    void CreateInstancedRectangles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* widths, const float* heights,
        int count);

    void CreateInstancedRectanglesTextured(const std::string& textureName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* widths, const float* heights,
        int count,
        const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f),
        const CEVec2& uvSize = CEVec2(1.0f, 1.0f));

    void RenderInstancedRectanglesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* widths,
        const float* heights,
        int count,
        const InstancedParams& params
    );

    void InitializeInstancedRectanglesData(int count);

    void InitializeEcsRectanglesDataRandom(int count);

    void RenderInstancedRectangles(const InstancedParams& params);

    void ClearRectangleData();
    

    //................Triangle................//

    void CreateInstancedTriangles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* sideLengths,
        int count);

    void CreateInstancedTrianglesTextured(const std::string& textureName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* sideLengths,
        int count,
        const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f),
        const CEVec2& uvSize = CEVec2(1.0f, 1.0f));
        

    void RenderInstancedTrianglesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* sideLengths,
        int count,
        const InstancingSystem::InstancedParams& params
    );

    void InitializeInstancedRectanglesData(int count);


    void InitializeMode4RandomShapes(int perTypeCount);

    

    // Build Mode 5 ECS-driven rectangles + triangles; circles generated directly
    void InitializeMode5EcsRandomShapes(
        CrashoutsEngine::ECS* ecs,
        int perTypeCount,
        bool texturesEnabled, const std::string& textureName,
        // Rectangles out
        std::vector<CEVec3>& rectPos, std::vector<CEVec3>& rectColor,
        std::vector<float>& rectRot, std::vector<float>& rectScale,
        std::vector<float>& rectW, std::vector<float>& rectH,
        // Triangles out
        std::vector<CEVec3>& triPos, std::vector<CEVec3>& triColor,
        std::vector<float>& triRot, std::vector<float>& triScale, std::vector<float>& triSide,
        // Circles out (not ECS)
        std::vector<CEVec3>& circPos, std::vector<CEVec3>& circColor,
        std::vector<float>& circRot, std::vector<float>& circScale, std::vector<float>& circRadius,
        int& circSegments
    );

    void InitializeMode4RandomShapes(int perTypeCount);
    void RenderMode4Rectangles(const InstancedParams& params);
    void RenderMode4Triangles(const InstancedParams& params);
    void RenderMode4Circles(const InstancedParams& params);
    
    void RenderInstancedRectangles(const InstancedParams& params);
    
    
    
    void InitializeEcsDuckSpritesRandom(int count);
    void UpdateEcsDuckSprites();
    void RenderEcsDuckSprites();
    void CycleEcsDuckAnimationState();
}