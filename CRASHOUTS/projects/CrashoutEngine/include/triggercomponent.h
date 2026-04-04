/*!
@file       triggercomponent.h
@author     m.biniswandi
@author     Shahrin Idzuan
@brief      Metadata component for triggers.
*/
#pragma once
#include <string>

namespace Component {
    /** @brief TriggerComponent */
    struct TriggerComponent {
        std::string triggerID;
        bool oneTimeUse = false;
        bool triggered = false;
    };
}
