/*!
@file       checkpointcomponent.h
@author     m.biniswandi
@author     Shahrin Idzuan
@brief      Checkpoint component for respawn logic.
*/
#pragma once
#include "CEVec3.h"

namespace Component {
    struct CheckpointComponent {
        CEVec3 respawnPosition{ 0.0f, 0.0f, 0.0f };
        uint32_t targetRespawnEntity = 0; // Runtime ID
        std::string targetEntityName;     // Serialized Name (for robust saving)
        bool activated = false;
    };
}
