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