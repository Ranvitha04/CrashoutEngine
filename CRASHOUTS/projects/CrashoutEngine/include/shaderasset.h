/*!
@file   shaderasset.h
@author Renfred (RTIS)
@author Shahrin Idzuan
@author Toastyship
@date   19/10/2025

@brief	This file contains the declaration of the shader

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */

#ifndef SHADER_H
#define SHADER_H

// LIBRARIES INCLUDES
#include <string>

#include "assetmanager.h"
#include "shader.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        class ShaderAsset : public IAsset {
        public:

            Component::GLSLShader shaderObject;

            // Default Constructor
            ShaderAsset() = default;

            // Deleted copy operations
            ShaderAsset(const ShaderAsset&) = delete;
            ShaderAsset& operator=(const ShaderAsset&) = delete;

            // Move constructor and assignment
            ShaderAsset(ShaderAsset&& other) = default;

            ShaderAsset& operator=(ShaderAsset&& other) = default;  

            // Default Destructor
            ~ShaderAsset() override = default;

            // Public member functions
            void Use() {
                shaderObject.Use();
            }

            void UnUse() {
                shaderObject.UnUse();
            }

            /*!
            @brief Factory method to create a new Asset from vertex and fragment files.
            @param vertexPath Path to the .vert file.
            @param fragmentPath Path to the .frag file.
            @return unique_ptr to ShaderAsset object or nullptr on failure
            */
            static std::unique_ptr<ShaderAsset> Create(const std::string& vertPath, const std::string& fragPath);
        };
    }
}

#endif // SHADER_H