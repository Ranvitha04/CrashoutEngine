/*!
@file       namecomponent.cs
@author     Renfred Lee
@date       29/11/2025
@brief      name component for entities in Crashouts Engine

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System;

namespace CrashoutsEngine
{
    [NativeComponent("AddName", "RemoveName", "EntityHasName")]
    public class NameComponent
    {
        public uint EntityID;

        public NameComponent() { }

        public string Value
        {
            get => InternalCalls.GetName(EntityID);
            set => InternalCalls.SetName(EntityID, value);
        }

        public override string ToString() => Value;
    }
}
