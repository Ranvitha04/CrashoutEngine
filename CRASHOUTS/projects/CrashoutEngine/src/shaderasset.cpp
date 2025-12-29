/*!    
@file   shaderasset.cpp
@author lee.r@digipen.edu
@date   16/09/2025

@brief	This file contains the definition of the ShaderAsset class.
         It loads shader paths from a JSON file and uses the
         GLSLShader class to compile and link them.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "shaderasset.h"
#include "shader.h" // For Component::GLSLShader
#include <iostream>
#include <vector> // For passing shader list to CompileLinkValidate

namespace CrashoutsEngine {

    namespace Assets {

        /*!
        @brief Creates a new ShaderAsset from vertex and fragment files.
        @param vertexPath Path to the .vert file.
        @param fragmentPath Path to the .frag file.
        @return Unique pointer to a valid ShaderAsset, or nullptr on failure.
        */
        std::unique_ptr<ShaderAsset> ShaderAsset::Create(const std::string& vertexPath, const std::string& fragmentPath) {

            // Create the asset
			std::unique_ptr<ShaderAsset> shaderAsset = std::make_unique<ShaderAsset>();

			// Use GLSLShader to compile and link the shader program
			std::vector<std::pair<GLenum, std::string>> shaderFiles = {
				{ GL_VERTEX_SHADER, vertexPath },
				{ GL_FRAGMENT_SHADER, fragmentPath }
			};

            if (GL_FALSE == shaderAsset->shaderObject.CompileLinkValidate(shaderFiles, true)) {
                std::cerr << "ShaderAsset::Create - Failed to compile/link shader from paths: "
                    << vertexPath << " and " << fragmentPath << std::endl;
                std::cerr << "GLSLShader Log: " << shaderAsset->shaderObject.GetLog() << std::endl;
                return nullptr; // Failed to create
            }

            std::cout << "Successfully loaded ShaderAsset: "
                << " (ID: " << shaderAsset->shaderObject.GetHandle() << ")" << std::endl;

            return shaderAsset;
        }
        
    }
}
