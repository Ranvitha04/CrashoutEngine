//#define APIENTRY
#undef APIENTRY
#include "engineinterface.h"
#include "Application.h"
#pragma comment(lib, "CrashoutEngine.lib")


namespace ScriptAPI
{
    void EngineInterface::HelloWorld()
    {
        // Managed output (C#-side)
        System::Console::WriteLine("Hello Managed World!");

        // Native call into your engine
        Core::Application::HelloWorld();
    }
}
