/*!
@file       audiocomponent.h
@author     mohamadamirakmal.b@digipen.edu
@date       25/10/2025
@brief	    This file defines the AudioComponent

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

#include <string>
#include <fmod_studio.hpp>
#include "CEVec3.h"

/*
----------------------------------------------------------------------------- */

namespace Component {
    struct AudioEmitterComponent {
        std::string eventPath;
        float volume = 1.0f;
        float pitch = 1.0f;
        bool spatial3D = false;
        bool looping = false;

        FMOD::Studio::EventInstance* inst = nullptr;
        FMOD::Channel* channel = nullptr;
    };
}