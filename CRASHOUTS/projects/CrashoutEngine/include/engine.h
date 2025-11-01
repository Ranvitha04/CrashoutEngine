/*!
@file    	engine.h
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file 

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#ifndef ENGINE_H
#define ENGINE_H

#include "glapp.h"
#include "performanceViewer.h"
#include "entitymanager.h"
#include "log.h"
#include "playerMovement.h"     // Player::UpdateADVelocity
#include "physicsmanager.h"            // Component::RigidBodyComponent
#include "inputmanager.h"
#include "debugreflection.h"
#include "debugmenu.h"
#include "crashlogger.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "frameratecontroller.h"
#include "gamestatemanager.h"
#include "registergamestate.h"
#include "settings.h"
#include <filesystem>

/*                                                      function declarations
----------------------------------------------------------------------------- */
void SetupConsole();
void Init();
void Update(Component::RigidBodyComponent&, double const&);
void Draw(CrashoutsEngine::Entity&);
void CleanUp();

// Engine States
enum class EngineState { STOP, PLAY, PAUSE };
extern EngineState engineState;

// For FBO globals
#ifdef USE_IMGUI_EDITOR
extern GLuint gameFboID;
extern GLuint gameTextureColorBuffer;
extern GLuint gameRboDepthStencil;
extern int gameTextureWidth;     // Will be resized
extern int gameTextureHeight;    // Will be resized

extern bool gameWinHovered;
#endif

#endif