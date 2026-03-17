/*!
@file       GhostController.cs
@author     Renfred (RTIS)
@author     Renfred Lee
@date       13/03/2026
@brief      Controls a ghost entity using recorded replay data.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Loads a saved ghost replay and applies playback transforms to this entity.
    */
    public class GhostController : ScriptBase
    {
        public string TrackName = "Level1";

        /*!
        @brief Load the ghost recording for the current track and start playback.
        */
        public override void Start()
        {
            bool ghostLoaded = ReplayManager.LoadRecording($"{TrackName}_Ghost");
            if (!ghostLoaded)
            {
                InternalCalls.SetActive(Entity.ID, false);
                return;
            }
            ReplayManager.StartPlayback();
        }

        /*!
        @brief Apply the latest ghost transform during fixed update ticks.
        */
        public override void FixedUpdate()
        {
            if (ReplayManager.GetGhostTransform(out CEVec3 savedPos, out float savedRot))
            {
                // GET THE LIVE TRANSFORM
                var liveTransform = Entity.GetComponent<TransformComponent>();
                //Debug.Log($"GHOST BRAIN: Trying to move to X: {savedPos.x}, Y: {savedPos.y}");
                // Set the position directly!
                liveTransform.Position = savedPos;
                liveTransform.Rotation = savedRot;
            }
        }
    }
}
