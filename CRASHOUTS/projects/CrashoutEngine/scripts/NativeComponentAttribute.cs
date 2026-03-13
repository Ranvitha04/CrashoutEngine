/*!
@file       NativeComponentAttribute.cs
@author     Renfred Lee
@date       29/11/2025
@brief      Native component attribute for marking native components

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System;

namespace CrashoutsEngine
{
    [AttributeUsage(AttributeTargets.Class)]
    public class NativeComponentAttribute : Attribute
    {
        public string AddCall { get; }
        public string RemoveCall { get; }
        public string HasCall { get; }

        public NativeComponentAttribute(string addCall, string removeCall, string hasCall)
        {
            AddCall = addCall;
            RemoveCall = removeCall;
            HasCall = hasCall;
        }
    }
}
