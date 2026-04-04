/*!
@file   audiocomponent.h
@author Mohamad Amir Akmal
@date   25/10/2025

@brief  This file defines the AudioComponent

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
        float minDistance = 1.0f;
        float maxDistance = 100.0f;
        int dspType = -1; // -1 for none, otherwise index into FMOD_DSP_TYPE

        // Internal handles (one per clip!)
        FMOD::Studio::EventInstance* inst = nullptr;
        FMOD::Channel* channel = nullptr;
    };

    /** @brief AudioEmitterComponent */
    struct AudioEmitterComponent {
        // The list of all sounds on this entity
        std::vector<AudioClip> clips;
    };
}