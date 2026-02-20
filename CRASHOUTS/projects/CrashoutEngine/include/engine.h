/*!
@file    	engine.h
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file 

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#ifndef ENGINE_H
#define ENGINE_H

#include "GL/glew.h"
#include "entitymanager.h"
#include "rigidbodycomponent.h"

/*                                                      function declarations
----------------------------------------------------------------------------- */
void SetupConsole();
void Init();
void Update(double const&);
void Draw(CrashoutsEngine::Entity&);
void CleanUp();

// Engine States
enum class EngineState { STOP, PLAY, PAUSE };
extern EngineState engineState;

// For FBO globals
extern GLuint gameFboID;
//extern GLuint gameTextureID;
extern GLuint gameDepthBufferID;
extern GLuint gameTextureColorBuffer;
extern GLuint gameRboDepthStencil;
extern int gameTextureWidth;     // Will be resized
extern int gameTextureHeight;    // Will be resized

#if USE_IMGUI_EDITOR
extern bool gameWinHovered;
#endif

extern std::string g_PendingSceneName;
extern bool g_IsSceneLoadPending;
extern bool g_ShowFPS;

extern std::string g_GameRootPath;
void SetGameRoot(const std::string& path);
std::string GetGameRoot();

std::string GetLocalAppData();

#endif