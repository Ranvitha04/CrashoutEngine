#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <GLFW/glfw3.h>
#include "key.h"


struct Player {
    float x = 0.0f;
    float y = 0.0f;
    float moveSpeed = 6.0f;   
};