using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddCheckpoint", "RemoveCheckpoint", "EntityHasCheckpoint")]
    public class CheckpointComponent
    {
        public uint EntityID;

        public CEVec3 RespawnPosition
        {
            get
            {
                InternalCalls.Checkpoint_GetRespawnPosition(EntityID, out CEVec3 pos);
                return pos;
            }
            set => InternalCalls.Checkpoint_SetRespawnPosition(EntityID, ref value);
        }

        public uint TargetRespawnEntity
        {
            get => InternalCalls.Checkpoint_GetTargetEntity(EntityID);
            set => InternalCalls.Checkpoint_SetTargetEntity(EntityID, value);
        }

        public string TargetEntityName
        {
            get => InternalCalls.Checkpoint_GetTargetName(EntityID);
            set => InternalCalls.Checkpoint_SetTargetName(EntityID, value);
        }

        public bool Activated
        {
            get => InternalCalls.Checkpoint_GetActivated(EntityID);
            set => InternalCalls.Checkpoint_SetActivated(EntityID, value);
        }
    }
}
