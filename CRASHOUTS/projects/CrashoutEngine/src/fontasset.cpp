/*!
@file   fontasset.cpp
@author Renfred Lee
@author Renfred (RTIS)
@author Shahrin Idzuan
@date   19/10/2025

@brief	This file contains the definition of the font asset class.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */

#include "fontasset.h"
#include "TextRenderer.h"
#include <iostream>

namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @brief Creates a new FontAsset from a json file.
        @param fontPath Path to the FontAsset(JSON) file.
        @return Unique pointer to a valid FontAsset, or nullptr on failure.
        */
        std::unique_ptr<FontAsset> FontAsset::Create(const std::string& fontPath) {

            Assets::AssetManager& assetManger = Assets::AssetManager::GetInstance();
            FT_Library ft = assetManger.GetFTLib();

            FT_Face face;
            if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
                std::cerr << "ERROR: Failed to load font at path: " << fontPath << "\n";
                return nullptr;
            }
            std::cout << "Loaded font: " << fontPath << "\n";

            FT_Set_Pixel_Sizes(face, 0, 48);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

            // Create the FontAsset object to store the characters
            std::unique_ptr<Assets::FontAsset> fontAsset = std::make_unique<Assets::FontAsset>(fontPath);

            // Load characters (ASCII 0 - 127)
            for (GLubyte c = 0; c < 128; c++) {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                    std::cerr << "ERROR::FREETYPE: Failed to load Glyph " << (char)c << "\n";
                    continue;
                }

                GLuint texture;
                glGenTextures(1, &texture);

                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
                );

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                // Store character
                Assets::Character character = {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<GLuint>(face->glyph->advance.x)
                };

                fontAsset->characters.insert(std::pair<GLchar, Assets::Character>(c, character));
            }

            glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
            FT_Done_Face(face); // Clean up the face

            std::cout << "Successfully loaded font asset: " << fontPath << "\n";

            return fontAsset;
        }
    }
}
