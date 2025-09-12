#pragma once

#include "CETypes.h"

enum class GameState : CE::u8 {
    QUIT = 0,
    MAIN_MENU,
    COUNT // dont remove this please, it used for array indexing - Rain 
};
