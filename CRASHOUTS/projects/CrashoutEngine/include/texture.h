///*!
//@file        texture.h
//@author      ranvitha@digipen.edu
//@date        05/05/2025
//
//This file declares the Texture struct, which provides static utility
//functions to create and draw textured quads. Each texture function
//encapsulates OpenGL buffer creation, attribute binding, and rendering
//configuration. Optional parameters allow transformations such as
//rotation, scaling, and uniform tinting.
//
//The Texture struct also provides a cleanup function to release
//rendering resources associated with texture rendering.
//
//*//*__________________________________________________________________________*/
//
///*                                                                   includes
//----------------------------------------------------------------------------- */


#ifndef TEXTURE_H
#define TEXTURE_H

/*!
@file        texture.h
@author      ranvitha@digipen.edu
@date        05/05/2025

This file declares the Texture struct, which provides static utility
functions to create and draw textured quads efficiently. The system
caches loaded textures and reuses a shared quad mesh to minimize
OpenGL state changes and memory allocations.

Optional parameters allow transformations such as rotation, scaling,
and uniform tinting.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "CEVec3.h"
#include "CEVec2.h"
#include <string>
#include <unordered_map>
#include <GL/glew.h>

namespace Component {

    struct Texture {

        // Function to create and draw a textured quad using texture file path
        // Automatically caches textures for reuse
        static void CreateTexturedQuad(const std::string& texturePath,
            const CEVec3& position,
            const CEVec2& size = CEVec2(1.0f, 1.0f),
            float rotation = 0.0f,
            float scale = 1.0f,
            const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f),
            bool modulate = false);

        // Function to create and draw a textured quad using texture handle
        static void CreateTexturedQuad(GLuint textureHandle,
            const CEVec3& position,
            const CEVec2& size = CEVec2(1.0f, 1.0f),
            float rotation = 0.0f,
            float scale = 1.0f,
            const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f),
            bool modulate = false);

        // Initialize the texture system (call once at startup)
        static void Initialize();

        // Preload textures for better performance
        static bool PreloadTexture(const std::string& texturePath);

        // Get texture handle (loads if not cached)
        static GLuint GetOrLoadTexture(const std::string& texturePath);

        // Utility function to load texture and return handle
        static GLuint LoadTexture(const std::string& pathname);

        // Resource management
        static void UnloadTexture(const std::string& texturePath);
        static void ClearTextureCache();

        // Function to clean up all texture rendering resources
        static void Cleanup();

    private:
        // Cached resources
        static std::unordered_map<std::string, GLuint> textureCache;
        static GLuint sharedVAO;
        static GLuint sharedVBO;
        static GLuint sharedEBO;
        static bool initialized;

        static GLuint LoadBinaryTexture(const std::string& pathname);
        static GLuint LoadImageTexture(const std::string& pathname);

        // Helper functions
        static void InitializeSharedQuad();
        static void RenderQuad(GLuint textureHandle,
            const CEVec3& position,
            const CEVec2& size,
            float rotation,
            float scale,
            const CEVec3& tint,
            bool modulate);
    };

}

#endif // TEXTURE_H