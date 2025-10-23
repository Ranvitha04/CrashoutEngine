/*!
@file		rendererconnector.h
@author     m.biniswandi@digipen.edu
@date       20/09/2025
@brief		Forward declaration for text renderers

*/
#pragma once
#include <memory>

class TextRenderer;
extern std::unique_ptr<TextRenderer> gTextRenderer;
extern std::unique_ptr<TextRenderer> gTextRendererA;