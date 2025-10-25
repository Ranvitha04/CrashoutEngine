/*!
@file		rendererconnector.cpp
@author     m.biniswandi@digipen.edu
@date       20/09/2025
@brief		Forward declaration for text renderers

*/
#include "rendererconnector.h"
#include "textrenderer.h"


std::unique_ptr<TextRenderer> gTextRenderer  = nullptr;
std::unique_ptr<TextRenderer> gTextRendererA = nullptr;