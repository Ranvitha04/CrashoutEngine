/*!
@file       scriptbase.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      the base class for all user-defined scripts in Crashouts Engine

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System;

namespace CrashoutsEngine
{
    public abstract class ScriptBase
    {
        // Set by the engine before Start() is called
        public Entity Entity;

        // Called once when the script instance is created
        public virtual void Start() { }

        // Called once per frame
        public virtual void Update() { }

        // Called at a fixed timestep
        public virtual void FixedUpdate() { }

        // Called for debug drawing (editor and play mode)
        public virtual void OnDrawGizmos() { }

        // Called when the script instance is being destroyed
        public virtual void OnDestroy() { }
    }
}
