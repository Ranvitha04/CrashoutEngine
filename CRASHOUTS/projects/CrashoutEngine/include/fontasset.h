/*!
@file   fontasset.h
@author lee.r@digipen.edu       
@date   19/10/2025

@brief	This file contains the declaration of the Font
//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */

#ifndef FONTASSET_H
#define FONTASSET_H

// LIBRARIES INCLUDES
#include <string>
#include <map>
#include "assetmanager.h"

#include FT_FREETYPE_H

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @struct Character
        @brief Stores glyph information for rendering text
        */
        struct Character {
            /// ID handle of the glyph texture
            GLuint TextureID;

            /// Size of the glyph in pixels
            glm::ivec2 Size;

            /// Offset from baseline to left and top
            glm::ivec2 Bearing;

            /// Horizontal offset to advance to the next glyph
            GLuint Advance;
        };

        class FontAsset : public IAsset {
        public:

            /// Map of loaded characters accessible by ASCII code
            std::map<GLchar, Character> Characters;

            // Default Constructor
            FontAsset() = default;

            // Custom Constructor 
            FontAsset(const std::string& fontName) :
                fontName(fontName) {
            }

            // Deleted copy operations
            FontAsset(const FontAsset&) = delete;
            FontAsset& operator=(const FontAsset&) = delete;

            // Move constructor and assignment
            FontAsset(FontAsset&& other) noexcept :
                fontName{std::move(other.fontName)}
            {};

            FontAsset& operator=(FontAsset&& other) noexcept
            {
                if (this != &other) {
                    fontName = std::move(other.fontName);
					other.fontName.clear();
                }
            }

			// Default Destructor
            ~FontAsset() = default;

            // Public member functions
            // GETTERS
			const std::string& GetFontName() const { return fontName; }

            /*!
            @brief Factory method to create a new Asset from file
            @param filePath Path to the FontAsset file to load
            @return unique_ptr to FontAsset object or nullptr on failure
            */
            static std::unique_ptr<FontAsset> Create(const std::string& filePath);
            
        private:
			std::string fontName;         // Name of the font
        };
    }

    namespace Serialization {
        bool LoadFont(std::string const& fontName);
    }
}

#endif // FONTASSET_H

