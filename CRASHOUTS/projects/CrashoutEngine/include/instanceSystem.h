#pragma once

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

    

    //glm::mat4 CreateCenteredWorldProjection();
}