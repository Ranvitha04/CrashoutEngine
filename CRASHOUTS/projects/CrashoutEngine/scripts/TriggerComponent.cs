/*!
@file       TriggerComponent.cs
@author     Renfred Lee
@date       13/03/2026
@brief      Managed wrapper for the native Trigger metadata component.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddTrigger", "RemoveTrigger", "EntityHasTrigger")]
    /*!
    @brief Exposes native trigger metadata to managed scripts.
    */
    public class TriggerMetadataComponent
    {
        public uint EntityID;

        /*!
        @brief Logical identifier used to route trigger events/logic.
        */
        public string TriggerID
        {
            get => InternalCalls.Trigger_GetID(EntityID);
            set => InternalCalls.Trigger_SetID(EntityID, value);
        }

        /*!
        @brief If true, the trigger can only be activated once.
        */
        public bool OneTimeUse
        {
            get => InternalCalls.Trigger_GetOneTimeUse(EntityID);
            set => InternalCalls.Trigger_SetOneTimeUse(EntityID, value);
        }

        /*!
        @brief Whether the trigger has already been activated.
        */
        public bool Triggered
        {
            get => InternalCalls.Trigger_GetTriggered(EntityID);
            set => InternalCalls.Trigger_SetTriggered(EntityID, value);
        }
    }
}
