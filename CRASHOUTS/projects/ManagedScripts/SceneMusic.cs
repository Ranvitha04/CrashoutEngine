/*!
@file		SceneMusic.cs
@author     mohamadamirakmal.b@digipen.edu
@date       27/01/2026
@brief		Declaration and implementation of SceneMusic script to manage background music per scene.
*/
using System;
using System.Runtime.CompilerServices;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SceneMusic : ScriptBase
    {

        [SerializeField] private string layerName = "Music";

        public override void Start()
        {
            string key, path;
            float vol, pitch;
            bool loop, isMusic;

            // Get Data from Component
            InternalCalls.Audio_GetComponentData(Entity.ID, out key, out path, out vol, out pitch, out loop, out isMusic);

            if (!string.IsNullOrEmpty(path) && !string.IsNullOrEmpty(key))
            {
                // Send to Manager with the specific LAYER NAME
                AudioManager.GetInstance().PlayGlobal(layerName, key, path, loop, vol);
            }
        }

        public override void Update() { }

        public virtual void FixedUpdate() { }

        public virtual void OnDrawGizmos() { }

        public virtual void OnDestroy() { }
    }
}
