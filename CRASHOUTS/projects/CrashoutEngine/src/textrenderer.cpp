/*!
@file       textrenderer.cpp
@author     m.biniswandi@digipen.edu
@date       25/09/2025
@brief      Implementation of the TextRenderer class
 
Handles font loading character atlas creation and text rendering with FreeType and OpenGL
*/

#include "TextRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "settings.h"

/*!
@brief Construct a new TextRenderer
@param fontPath Path to the font file
@param screenWidth Screen width used
@param screenHeight Screen height used
*/
TextRenderer::TextRenderer(const std::string& fontPath, GLuint screenWidth, GLuint screenHeight){
    shaderID = LoadShaderProgram("assets/shaders/textShader.vert", "assets/shaders/textShader.frag");
    if (shaderID == 0) {
        std::cerr << "Failed to load text shader.\n";
        return;
    }
    /// Configure VAO/VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /// Set projection matrix
    glUseProgram(shaderID);
    glm::mat4 projection = glm::ortho(
		-static_cast<float>(screenWidth) / 2.0f, // left edge
        static_cast<float>(screenWidth) / 2.0f,  // right edge
        static_cast<float>(screenHeight) / 2.0f,// bottom edge
        -static_cast<float>(screenHeight) / 2.0f  // top edge
    );


    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, &projection[0][0]);

    LoadFont(fontPath);
}

/// Destructor that deletes the VAO and VBO
TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    for (auto& kv : Characters) {
        glDeleteTextures(1, &kv.second.TextureID);
    }
    Characters.clear();

    // Delete VAO/VBO
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // Delete shader program
    if (shaderID != 0) {
        glDeleteProgram(shaderID);
    }
}

// MOVED TO FONTASSET_H
#if FALSE 
/*!
@brief Load a font and create character textures
@param fontPath Path to the font file
@return True if font loaded successfully false otherwise
*/
bool TextRenderer::LoadFont(const std::string& fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR: Could not init FreeType Library\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR: Failed to load font at path: " << fontPath << "\n";
        return false;
    }
    std::cout << "Loaded font: " << fontPath << "\n";


    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR: Failed to load Glyph\n";
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

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };

        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}
#endif

/*!
@brief Render text at a given position
@param text String to render
@param x Left aligned X position in screen space
@param y Baseline Y position in screen space
@param scale Scale factor for text size
@param color RGB color of the text
*/
void TextRenderer::RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    glUseProgram(shaderID);
    glUniform3f(glGetUniformLocation(shaderID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (auto c : text) {
        Character ch = Characters[c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - ch.Bearing.y * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        GLfloat vertices[6][4] = {
        { xpos,     ypos + h,   0.0f, 1.0f },
        { xpos,     ypos,       0.0f, 0.0f },
        { xpos + w, ypos,       1.0f, 0.0f },

        { xpos,     ypos + h,   0.0f, 1.0f },
        { xpos + w, ypos,       1.0f, 0.0f },
        { xpos + w, ypos + h,   1.0f, 1.0f }
        };

        // Enable blending
        glEnable(GL_BLEND);

        // Set blend function
        // Use the incoming fragment alpha 
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*!
@brief Render centered text
@param text String to render
@param x Center X position
@param y Center Y position
@param scale Scale factor for text size
@param color RGB color of the text

Measures text dimensions and adjusts position so that text is centered on the given coordinates
*/
void TextRenderer::RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    // Measure total width and max full height (including descent)
    GLfloat textWidth = 0.0f;
    GLfloat maxAscent = 0.0f;
    GLfloat maxDescent = 0.0f;

    for (char c : text) {
        Character ch = Characters[c];
        textWidth += (ch.Advance >> 6) * scale;

        GLfloat ascent = ch.Bearing.y * scale;
        GLfloat descent = (ch.Size.y - ch.Bearing.y) * scale;

        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;
    }

    GLfloat originX = x - textWidth / 2.0f;
    GLfloat originY = y + (maxAscent - (maxAscent + maxDescent) / 2.0f);

    RenderText(text, originX, originY, scale, color);
}

/*!
@brief Compile a shader from source code
@param type OpenGL shader type such as GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
@param source GLSL shader source code
@return Compiled shader ID or 0 if compilation fails
*/
GLuint CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    /// Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

/*!
@brief Read the contents of a file into a string
@param path Path to the file
@return File contents as a string or empty string if reading fails
*/
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;

    if (!file.is_open()) {
        std::cerr << "ERROR::FILE_NOT_READABLE: " << path << std::endl;
        return "";
    }

    buffer << file.rdbuf();
    return buffer.str();
}

/*!
@brief Load and link a shader program from vertex and fragment shader files
@param vertexPath Path to the vertex shader file
@param fragmentPath Path to the fragment shader file
@return Linked shader program ID or 0 if linking fails
*/
GLuint LoadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = ReadFile(vertexPath);
    std::string fragmentCode = ReadFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty())
        return 0;

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    /// Check linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << std::endl;
    }

    /// Clean up shaders (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
