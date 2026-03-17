/*!
@file		rendererconnector.h
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@author     Ranvitha
@date       20/09/2025
@brief		Forward declaration for text renderers

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <memory>

class TextRenderer;
extern std::unique_ptr<TextRenderer> gTextRenderer;
extern std::unique_ptr<TextRenderer> gTextRendererA;