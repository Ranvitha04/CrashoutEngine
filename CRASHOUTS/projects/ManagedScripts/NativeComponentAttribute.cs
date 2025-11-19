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
