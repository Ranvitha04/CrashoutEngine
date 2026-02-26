/*!
@file       triggercomponent.h
@author     m.biniswandi@digipen.edu
@brief      Metadata component for triggers.
*/
#pragma once
#include <string>

namespace Component {
    struct TriggerComponent {
        std::string triggerID;
        bool oneTimeUse = false;
        bool triggered = false;
    };
}
