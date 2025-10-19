#pragma once

namespace ScriptAPI
{
    // 'ref class' means it's a managed class accessible from .NET
    public ref class EngineInterface
    {
    public:
        static void HelloWorld();   // simple test call
    };
}
