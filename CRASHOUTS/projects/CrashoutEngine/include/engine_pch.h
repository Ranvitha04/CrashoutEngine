// This file will be precompiled

#pragma once

// C++ Standard Library
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

// Vendor Libraries
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include <rapidjson/document.h>
#include <imgui.h>
#include <fmod.hpp>
#include <ft2build.h>
#include <glm/gtc/matrix_transform.hpp>

// Your Own Engine Headers
// (Only add the most stable ones that are used *everywhere*)
#include "entitymanager.h" 
#include "assetmanager.h"
#include "textureasset.h"
#include "fontasset.h"
#include "shaderasset.h"
#include "sceneasset.h"
#include "CEVec2.h"
#include "CEVec3.h"
#include "CETypes.h"
#include "CEMtx33.h"
#include "CEMath.h"
#include "cameracomponent.h"
#include "transformcomponent.h"
#include "textcomponent.h"
#include "namecomponent.h"
#include "performanceViewer.h"
#include "frameratecontroller.h"
#include "glapp.h"
// ...