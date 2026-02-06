/*!
@file       SerializeField.cs
@author     lee.r@digipen.edu
@date       27/11/2025
@brief      Defines the SerializeField attribute for marking fields to be serialized.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    // Defines the [SerializeField] attribute
    // We restrict it to Fields only, just like Unity
    [AttributeUsage(AttributeTargets.Field)]
    public class SerializeField : Attribute
    {
    }
}
