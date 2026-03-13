/*!
@file       textrenderer.h
@author     Shahrin Idzuan
@author     Renfred Lee
@author     Renfred (RTIS)
@date       25/09/2025
@brief      Declaration of the TextRenderer class

Provides text rendering using FreeType and OpenGL

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
*/

#pragma once


#include <string>


#include <glm/gtc/matrix_transform.hpp>
#include "GL/glew.h"
#include "glm/vec3.hpp"

#include "CEVec3.h"
#include "CEColor.h"
#include "CEVec2.h"

#include "fontasset.h"

// MOVED TO (FONTASSET_H)
#if FALSE 
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
#endif

/*!
@class TextRenderer
@brief Renders text using FreeType and OpenGL

Manages font loading character glyphs and rendering text in screen space
*/
class TextRenderer {
public:
    /// Map of loaded characters accessible by ASCII code 
    // std::map<GLchar, Character> Characters; (MOVED TO FONTASSET_H)

    /// Vertex array object for text quads
    GLuint VAO;

    /// Vertex buffer object for text quads
    GLuint VBO;

    /// Shader program ID for text rendering
    GLuint shaderID;

    /*!
    @brief Construct a new TextRenderer
    */
    TextRenderer();

    /// Destructor that cleans up buffers
    ~TextRenderer() {

    }

    /*!
    @brief Render left aligned text
    @param text String to render
    @param x Left aligned X position in screen space
    @param y Baseline Y position in screen space
    @param z Center Z position in screen space
    @param scale Scale factor for text size
    @param color RGB color of the text
    @param fontName The name of the font asset (in AssetManager) to use
    */
    void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat z, GLfloat scale, CEColor color, const std::string fontName = "Exo2-Regular", bool isWorldSpace = false, float letterSpacing = 0.0f);
   
    /*!
    @brief Render centered text
    @param text String to render
    @param x Center X position in screen space
    @param y Center Y position in screen space
    @param z Center Z position in screen space
    @param scale Scale factor for text size
    @param color RGB color of the text
    @param fontName The name of the font asset (in AssetManager) to use
    */
    void RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat z, GLfloat scale, CEColor color, const std::string fontName = "Exo2-Regular", bool isWorldSpace = false, float letterSpacing = 0.0f);

    /*!
    @brief Render text within a bounding box with wrapping and alignment
    */
    void RenderTextBounded(const std::string& text, 
                           GLfloat x, GLfloat y, GLfloat z, 
                           GLfloat maxWidth, GLfloat maxHeight, 
                           GLfloat scale, CEColor color, 
                           const std::string fontName, 
                           bool isWorldSpace,
                           int alignment, int verticalAlignment,
                           CEVec2 padding = {0.f, 0.f},
                           float letterSpacing = 0.0f,
                           float lineSpacing = 0.0f);

    // Function to get the (x, y) baseline position of the cursor
    CEVec2 GetCursorPosition(const std::string& text, size_t cursorPos, 
        GLfloat x, GLfloat y, GLfloat scale, const std::string fontName, float letterSpacing = 0.0f);

    // Helper to get font height (we'll need this for drawing)
    GLfloat GetLineHeight(const std::string fontName, GLfloat scale);

    float GetTextWidth(const std::string& text, const std::string& fontName, GLfloat scale, float letterSpacing = 0.0f);

private:
    /*!
    @brief Load a font and create glyph textures
    @param fontPath Path to the font file
    @return True if loaded successfully false otherwise
    */
    //bool LoadFont(const std::string& fontPath); (MOVED TO FONTASSET_H)
};
/*!
@brief Load and link a shader program from vertex and fragment shader files
@param vertexPath Path to the vertex shader file
@param fragmentPath Path to the fragment shader file
@return Linked shader program ID or 0 if linking fails
*/
GLuint LoadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);