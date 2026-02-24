using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddTrigger", "RemoveTrigger", "EntityHasTrigger")]
    public class TriggerMetadataComponent
    {
        public uint EntityID;

        public string TriggerID
        {
            get => InternalCalls.Trigger_GetID(EntityID);
            set => InternalCalls.Trigger_SetID(EntityID, value);
        }

        public bool OneTimeUse
        {
            get => InternalCalls.Trigger_GetOneTimeUse(EntityID);
            set => InternalCalls.Trigger_SetOneTimeUse(EntityID, value);
        }

        public bool Triggered
        {
            get => InternalCalls.Trigger_GetTriggered(EntityID);
            set => InternalCalls.Trigger_SetTriggered(EntityID, value);
        }
    }
}
