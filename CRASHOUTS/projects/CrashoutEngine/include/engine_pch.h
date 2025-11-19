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
//#include <GLFW/glfw3.h>

// RapidJSON
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>

#include <imgui.h>
#include <fmod.hpp>
#include <ft2build.h>

// Your Own Engine Headers
// (Only add the most stable ones that are used *everywhere*)
#include "entitymanager.h" 
#include "assetmanager.h"
// ...