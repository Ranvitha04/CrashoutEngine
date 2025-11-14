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
//#include <rapidjson/document.h>
#include <imgui.h>
//#include <GLFW/glfw3.h>
#include <fmod.hpp>
#include <cstdio>
#include <crtdbg.h>

// Your Own Engine Headers
// (Only add the most stable ones that are used *everywhere*)
#include "entitymanager.h" 
#include "assetmanager.h"
#include "CEVec2.h"
#include "CEVec3.h"
#include "engine.h"
#include "transformcomponent.h"
#include "performanceViewer.h"
#include "frameratecontroller.h"
#include "glapp.h"
// ...