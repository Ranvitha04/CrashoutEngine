/*!
@file		rendererconnector.cpp
@author     Shahrin Idzuan
@date       20/09/2025
@brief		Forward declaration for text renderers

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "rendererconnector.h"
#include "textrenderer.h"


std::unique_ptr<TextRenderer> gTextRenderer  = nullptr;
std::unique_ptr<TextRenderer> gTextRendererA = nullptr;