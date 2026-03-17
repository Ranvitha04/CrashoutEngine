/*!
@file       ScriptBehaviourAttribute.cs
@author     Renfred Lee
@date       25/11/2025
@brief      Defines the ScriptBehaviourAttribute class used to mark script behaviour classes.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    [AttributeUsage(AttributeTargets.Class)]
    public class ScriptBehaviourAttribute : Attribute
    {
    }
}
