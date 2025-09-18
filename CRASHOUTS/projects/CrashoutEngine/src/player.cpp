// player.cpp
#include "playerMovement.h"
#include <iostream>

namespace Player {
    //Take Component
    void UpdateVelocity(Component::RigidBodyComponent& rb, float moveSpeed)
    {
        //direction
        float h = 0.0f;
        //A is move left D is move right 
        if (InputManager::IsKeyPressed(Key::A)) h -= 1.0f;
        if (InputManager::IsKeyPressed(Key::D)) h += 1.0f;

        //how fast and direction
        rb.setVelocity(CEVec3{ h * moveSpeed, 0.f, 0.f });

        // show how much velocity is being used this frame
        //std::cout <<  rb.getVelocity().x << " Units per second\n";
    }
}

