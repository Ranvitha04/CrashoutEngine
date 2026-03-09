/*!
@file       textrenderer.cpp
@author     m.biniswandi@digipen.edu    (80%)
@co-author  lee.r@digipen.edu           (20%)
@date       25/09/2025
@brief      Implementation of the TextRenderer class

Handles font loading character atlas creation and text rendering with FreeType and OpenGL

All content  2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "rendering.h"
#include "TextRenderer.h"
#include "shapes.h"
#include "glapp.h"
#include <CEVec3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "engine.h"

#include "settings.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include "FontAsset.h"
#include "camera.h"

#define FONT_SCALE 0.05f
#define INV_FONT_SCALE 20.f

/*!
@brief Construct a new TextRenderer
*/
TextRenderer::TextRenderer() {
    using namespace CrashoutsEngine::Assets;

    ShaderAsset* textureShader = AssetManager::GetInstance().Get<ShaderAsset>("TextShader");

    if (!textureShader) {
        std::cerr << "ERROR::TEXTRENDERER: Could not find TextShader asset\n";
        shaderID = 0;
        return;
    }

    shaderID = textureShader->shaderObject.GetHandle();

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
    //glUseProgram(shaderID);
    //
    //Component::Rendering& renderer = Component::Rendering::Get();
    //glm::mat4 projection = renderer.projectionMatrix;
    //
    //glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, &projection[0][0]);

    //LoadFont(fontPath);
}

/*!
@brief Render text at a given position
@param text String to render
@param x Left aligned X position in screen space
@param y Baseline Y position in screen space
@param scale Scale factor for text size
@param color RGB color of the text
@param fontName The name of the font asset (in AssetManager) to use
*/
void TextRenderer::RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat z, GLfloat scale, CEColor colorCEV, const std::string fontName, bool isWorldSpace, float letterSpacing) {
    scale *= FONT_SCALE;

    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);

    if (!font) {
        std::cerr << "ERROR::TEXTRENDERER: Font not found: " << fontName << std::endl;
        return;
    }
    // Get character map
    const std::map<GLchar, Character>& characters = font->characters;

    glUseProgram(shaderID);

    // Get the *current* camera matrices every frame
    glm::mat4 projection;
    glm::mat4 view;

    if (isWorldSpace) {
        // WORLD SPACE: Use the game's moving camera

        if (engineState == EngineState::STOP) {
            projection = CrashoutsEngine::RenderCamera::engineCamera.GetProjectionMatrix();
            view = CrashoutsEngine::RenderCamera::engineCamera.GetViewMatrix();
        }
        else {
            projection = CrashoutsEngine::RenderCamera::gameCamera.GetProjectionMatrix();
            view = CrashoutsEngine::RenderCamera::gameCamera.GetViewMatrix();
        }

    }
    else {
        // SCREEN SPACE: Use the static UI camera
        //Component::Rendering& renderer = Component::Rendering::Get();
        projection = glm::ortho(
            0.0f, static_cast<float>(GLApp::width),
            0.0f, static_cast<float>(GLApp::height),
            -1.0f, 1.0f
        );

        // 2. View is Identity (No camera movement)
        view = glm::mat4(1.0f);
    }

    // Send them to the text shader
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, &view[0][0]);
    glUniform1f(glGetUniformLocation(shaderID, "uDepth"), z);

    glUniform4f(glGetUniformLocation(shaderID, "textColor"), colorCEV.r, colorCEV.g, colorCEV.b, colorCEV.a);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // --- Store the original X position for line breaks ---
    GLfloat original_x = x;

    // --- Get a line height ---
    // This value *should* come from your font loading.
    // It's (face->size->metrics.height >> 6) in FreeType.
    // You should store this in your FontAsset.
    // For now, we'll try to get 'H's height as a good guess.
    GLfloat baseLineHeight = 64.0f; // A default fallback
    if (characters.count('H')) {
        baseLineHeight = (float)characters.at('H').Size.y;
    }
    else if (!characters.empty()) {
        baseLineHeight = (float)characters.begin()->second.Size.y; // Less reliable
    }

    // Your math (y + Bearing, y - h) implies Y is the baseline and positive-Y is UP.
    // So, we must DECREASE Y to go to the next line.
    // We'll use 1.2x the base height for spacing.
    GLfloat lineHeight = (baseLineHeight * 1.2f) * scale;

    for (auto c : text) {

        // Check for newline
        if (c == '\n') {
            y -= lineHeight;        // Move Y down
            x = original_x;     // Reset X to the start
            continue;           // Skip the rest of this loop
        }

        // Check if character is in the map (to prevent crash)
        if (characters.find(c) == characters.end()) {
            // If char not found, you could skip it or draw a '?'
            continue;
        }

        Character ch = characters.at(c);

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y + ch.Bearing.y * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        GLfloat vertices[6][4] = {
        { xpos,     ypos - h,   0.0f, 1.0f },
        { xpos,     ypos,       0.0f, 0.0f },
        { xpos + w, ypos,       1.0f, 0.0f },

        { xpos,     ypos - h,   0.0f, 1.0f },
        { xpos + w, ypos,       1.0f, 0.0f },
        { xpos + w, ypos - h,   1.0f, 1.0f }
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
        Component::Rendering::Get().drawCalls++;
        x += (ch.Advance >> 6) * scale + (letterSpacing * scale); // Apply letter spacing
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
@param fontName The name of the font asset (in AssetManager) to use

Measures text dimensions and adjusts position so that text is centered on the given coordinates
*/
void TextRenderer::RenderCenteredText(const std::string& text, GLfloat x, GLfloat y, GLfloat z, GLfloat scale, CEColor color, const std::string fontName, bool isWorldSpace, float letterSpacing) {
    scale *= FONT_SCALE;
    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);

    if (!font) {
        std::cerr << "ERROR::TEXTRENDERER: Font not found: " << fontName << std::endl;
        return;
    }
    // Get character map
    const std::map<GLchar, Character>& characters = font->characters;

    // Measure total width and max full height (including descent)
    GLfloat textWidth = 0.0f;
    GLfloat maxAscent = 0.0f;
    GLfloat maxDescent = 0.0f;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        Character ch = characters.at(c);
        textWidth += (ch.Advance >> 6) * scale;
        if (i < text.length() - 1) textWidth += (letterSpacing * scale);

        GLfloat ascent = ch.Bearing.y * scale;
        GLfloat descent = (ch.Size.y - ch.Bearing.y) * scale;

        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;
    }

    GLfloat originX = x - textWidth / 2.0f;
    GLfloat originY = y - (maxAscent - (maxAscent + maxDescent) / 2.0f);

    RenderText(text, originX, originY, z, scale * INV_FONT_SCALE, color, fontName, isWorldSpace, letterSpacing);
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

// This is the helper to get a line height
GLfloat TextRenderer::GetLineHeight(const std::string fontName, GLfloat scale) {
    scale *= FONT_SCALE;

    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);
    if (!font) return 20.0f; // Default fallback

    const std::map<GLchar, Character>& characters = font->characters;

    GLfloat baseLineHeight = 64.0f; // A default fallback
    if (characters.count('H')) {
        baseLineHeight = (float)characters.at('H').Size.y;
    }
    else if (!characters.empty()) {
        baseLineHeight = (float)characters.begin()->second.Size.y;
    }

    return (baseLineHeight * 1.2f) * scale;
}

float TextRenderer::GetTextWidth(const std::string& text, const std::string& fontName, GLfloat scale, float letterSpacing) {
    scale *= FONT_SCALE;
    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);

    if (!font) return 0.0f;

    const std::map<GLchar, Character>& characters = font->characters;
    float textWidth = 0.0f;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (characters.find(c) != characters.end()) {
            Character ch = characters.at(c);
            textWidth += (ch.Advance >> 6) * scale;
            if (i < text.length() - 1) {
                textWidth += (letterSpacing * scale);
            }
        }
    }
    return textWidth;
}

// This is the main calculation function
CEVec2 TextRenderer::GetCursorPosition(const std::string& text, size_t cursorPos,
    GLfloat x, GLfloat y, GLfloat scale,
    const std::string fontName, float letterSpacing) // <-- ADDED letterSpacing parameter!
{
    scale *= FONT_SCALE;

    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);

    if (!font) {
        return CEVec2{ x, y }; // Return default on error
    }
    const std::map<GLchar, Character>& characters = font->characters;

    GLfloat original_x = x;

    // Fix Double-Scaling: scale was already multiplied by FONT_SCALE above, 
    // and GetLineHeight ALSO multiplies by FONT_SCALE internally. Pass the raw scale!
    GLfloat lineHeight = GetLineHeight(fontName, scale / FONT_SCALE);

    // Loop only up to the cursor's position
    for (size_t i = 0; i < cursorPos; ++i) {
        char c = text[i];

        if (c == '\n') {
            y -= lineHeight; // RenderText uses full lineHeight, not .5f
            x = original_x;
            continue;
        }

        if (characters.find(c) == characters.end()) {
            continue; // Skip unknown chars
        }

        Character ch = characters.at(c);

        // Removed the * .5f, and added letterSpacing to exactly match RenderText!
        x += (ch.Advance >> 6) * scale + (letterSpacing * scale);
    }

    // Return the final (x, y) baseline position
    return CEVec2{ x, y };
}

/*!
@brief Render bounded and wrapped text
*/
void TextRenderer::RenderTextBounded(const std::string& text, 
                                     GLfloat x, GLfloat y, GLfloat z, 
                                     GLfloat maxWidth, GLfloat maxHeight, 
                                     GLfloat scale, CEColor color, 
                                     const std::string fontName, 
                                     bool isWorldSpace,
                                     int alignment, int verticalAlignment,
                                     CEVec2 padding,
                                     float letterSpacing, float lineSpacing) 
{
    // Alignment: 0=LEFT, 1=CENTER, 2=RIGHT
    // VertAlign: 0=TOP, 1=MIDDLE, 2=BOTTOM

    // Apply Padding to the Box Size
    float effWidth = maxWidth - (padding.x * 2.0f);
    //float effHeight = maxHeight - (padding.y * 2.0f);

    if (effWidth <= 0.0f) effWidth = 1.0f;

    float renderScale = scale * FONT_SCALE;
    using namespace CrashoutsEngine::Assets;
    AssetManager& assetManager = AssetManager::GetInstance();
    FontAsset* font = assetManager.Get<FontAsset>(fontName);

    if (!font) {
        std::cerr << "ERROR::TEXTRENDERER: Font not found: " << fontName << std::endl;
        return;
    }
    const std::map<GLchar, Character>& characters = font->characters;

    // 1. Calculate Line Height
    GLfloat baseLineHeight = 64.0f;
    if (characters.count('H')) baseLineHeight = (float)characters.at('H').Size.y;
    else if (!characters.empty()) baseLineHeight = (float)characters.begin()->second.Size.y;
    
    GLfloat lineHeight = ((baseLineHeight * 1.2f) + lineSpacing) * renderScale;
    
    GLfloat spaceWidth = 0.0f;
    if (characters.count(' ')) spaceWidth = (characters.at(' ').Advance >> 6) * renderScale + (letterSpacing * renderScale);
    else spaceWidth = (baseLineHeight * 0.4f) * renderScale;

    // 2. Word Wrap Logic (Space-based only)
    std::vector<std::string> lines;
    std::vector<float> lineWidths;

    std::string currentLine = "";
    float currentLineWidth = 0.0f;
    bool firstWord = true;
    
    size_t start = 0;
    while (start < text.length()) {
        size_t end = text.find('\n', start);
        std::string paragraph = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        
        std::stringstream pss(paragraph);
        std::string pWord;
        currentLine = "";
        currentLineWidth = 0.0f;
        firstWord = true;

        while (pss >> pWord) {
            float wordWidth = 0.0f;
            for (size_t i = 0; i < pWord.length(); ++i) {
                char c = pWord[i];
                if (characters.count(c)) {
                    wordWidth += (characters.at(c).Advance >> 6) * renderScale;
                    if (i < pWord.length() - 1) wordWidth += (letterSpacing * renderScale);
                }
            }

            if (firstWord) {
                currentLine = pWord;
                currentLineWidth = wordWidth;
                firstWord = false;
            } else {
                if (currentLineWidth + spaceWidth + wordWidth <= effWidth) {
                    currentLine += " " + pWord;
                    currentLineWidth += spaceWidth + wordWidth;
                } else {
                    lines.push_back(currentLine);
                    lineWidths.push_back(currentLineWidth);
                    currentLine = pWord;
                    currentLineWidth = wordWidth;
                }
            }
        }
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
            lineWidths.push_back(currentLineWidth);
        } else if (paragraph.empty() && end != std::string::npos) {
             lines.push_back("");
             lineWidths.push_back(0);
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }

    // 3. Calculate Vertical Start
    float totalTextHeight = lines.size() * lineHeight;
    
    // Bounds edges (assuming x,y is center of box)
    // Adjust box edges by padding
    float boxTop = (y + maxHeight * 0.5f) - padding.y;
    float boxBottom = (y - maxHeight * 0.5f) + padding.y;
    float boxLeft = (x - maxWidth * 0.5f) + padding.x;
    float boxRight = (x + maxWidth * 0.5f) - padding.x;

    float startY = y;

    if (verticalAlignment == 0) { // TOP
        startY = boxTop - lineHeight; // Start drawing first line
    } else if (verticalAlignment == 1) { // MIDDLE
        // Center the text block around Y
        float effCenterY = y; // Assuming margins are symmetric for center alignment
        float blockTop = effCenterY + (totalTextHeight * 0.5f);
        startY = blockTop - lineHeight * 0.8f; 
    } else { // BOTTOM
        startY = boxBottom + totalTextHeight - lineHeight * 0.8f;
    }

    // 4. Render Lines
    for (size_t i = 0; i < lines.size(); ++i) {
        float lineX = boxLeft;
        if (alignment == 1) { // CENTER
            lineX = x - (lineWidths[i] * 0.5f); 
        } else if (alignment == 2) { // RIGHT
            lineX = boxRight - lineWidths[i];
        }

        RenderText(lines[i], lineX, startY, z, scale, color, fontName, isWorldSpace, letterSpacing);
        startY -= lineHeight;
    }
}