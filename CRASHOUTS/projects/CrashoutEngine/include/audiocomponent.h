/*!
@file       audiocomponent.h
@author     mohamadamirakmal.b@digipen.edu
@date       25/10/2025
@brief	    This file defines the AudioComponent

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

        // Runtime
        FMOD::Studio::EventInstance* inst = nullptr;
    };
}