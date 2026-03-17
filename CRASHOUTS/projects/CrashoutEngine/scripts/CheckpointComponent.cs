/*!
@file       CheckpointComponent.cs
@author     Renfred Lee
@date       13/03/2026
@brief      Managed wrapper for the native Checkpoint component.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddCheckpoint", "RemoveCheckpoint", "EntityHasCheckpoint")]
    /*!
    @brief Exposes native checkpoint data to managed scripts.
    */
    public class CheckpointComponent
    {
        public uint EntityID;

        /*!
        @brief Respawn position to use when this checkpoint is activated.
        */
        public CEVec3 RespawnPosition
        {
            get
            {
                InternalCalls.Checkpoint_GetRespawnPosition(EntityID, out CEVec3 pos);
                return pos;
            }
            set => InternalCalls.Checkpoint_SetRespawnPosition(EntityID, ref value);
        }

        /*!
        @brief Entity id to respawn/teleport when this checkpoint is activated.
        */
        public uint TargetRespawnEntity
        {
            get => InternalCalls.Checkpoint_GetTargetEntity(EntityID);
            set => InternalCalls.Checkpoint_SetTargetEntity(EntityID, value);
        }

        /*!
        @brief Target entity name used for editor/debug display.
        */
        public string TargetEntityName
        {
            get => InternalCalls.Checkpoint_GetTargetName(EntityID);
            set => InternalCalls.Checkpoint_SetTargetName(EntityID, value);
        }

        /*!
        @brief Whether this checkpoint has been activated.
        */
        public bool Activated
        {
            get => InternalCalls.Checkpoint_GetActivated(EntityID);
            set => InternalCalls.Checkpoint_SetActivated(EntityID, value);
        }
    }
}
