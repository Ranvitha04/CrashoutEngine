/**
 * @file TextRenderer.h
 * @brief Declaration of the TextRenderer class
 *
 * Provides text rendering using FreeType and OpenGL
 */

#pragma once


#include <map>
#include <string>

#include <ft2build.h>

#include "glm/vec2.hpp"
#include "GL/glew.h"
#include "glm/vec3.hpp"

#include FT_FREETYPE_H


 /**
  * @struct Character
  * @brief Stores glyph information for rendering text
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

/**
 * @class TextRenderer
 * @brief Renders text using FreeType and OpenGL
 *
 * Manages font loading character glyphs and rendering text in screen space
 */
class TextRenderer {
public:
    /// Map of loaded characters accessible by ASCII code
    std::map<GLchar, Character> Characters;

    /// Vertex array object for text quads
    GLuint VAO;

    /// Vertex buffer object for text quads
    GLuint VBO;

    /// Shader program ID for text rendering
    GLuint shaderID;

    /**
     * @brief Construct a new TextRenderer
     * @param fontPath Path to the font file
     * @param screenWidth Screen width used for projection
     * @param screenHeight Screen height used for projection
     */
    TextRenderer(const std::string& fontPath, GLuint screenWidth, GLuint screenHeight);

    /// Destructor that cleans up buffers
    ~TextRenderer();

    /**
     * @brief Render left aligned text
     * @param text String to render
     * @param x Left aligned X position in screen space
     * @param y Baseline Y position in screen space
     * @param scale Scale factor for text size
     * @param color RGB color of the text
     */
    void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
   
    /**
     * @brief Render centered text
     * @param text String to render
     * @param x Center X position in screen space
     * @param y Center Y position in screen space
     * @param scale Scale factor for text size
     * @param color RGB color of the text
     */
    void RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);


private:
    /**
     * @brief Load a font and create glyph textures
     * @param fontPath Path to the font file
     * @return True if loaded successfully false otherwise
     */
    bool LoadFont(const std::string& fontPath);
};
/**
 * @brief Load and link a shader program from vertex and fragment shader files
 * @param vertexPath Path to the vertex shader file
 * @param fragmentPath Path to the fragment shader file
 * @return Linked shader program ID or 0 if linking fails
 */
GLuint LoadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);