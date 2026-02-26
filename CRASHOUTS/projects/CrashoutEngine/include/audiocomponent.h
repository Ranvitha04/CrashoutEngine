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
	// Single audio clip definition
    struct AudioClip {
        std::string name = "New Clip"; // The name you give it (e.g. "Jump")
        std::string path = "";         // The file path
        float volume = 1.0f;
        float pitch = 1.0f;
        bool looping = false;
        bool isMusic = false;
        bool spatial3D = false;

        // Internal handles (one per clip!)
        FMOD::Studio::EventInstance* inst = nullptr;
        FMOD::Channel* channel = nullptr;
    };

    struct AudioEmitterComponent {
        // The list of all sounds on this entity
        std::vector<AudioClip> clips;
    };
}