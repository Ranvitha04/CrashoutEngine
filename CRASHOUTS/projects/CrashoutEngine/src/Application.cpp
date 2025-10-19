#include "Application.h"
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
namespace Core
{
    void Application::Run()
    {
        // Start your engine normally
        SetupConsole();
        Init();

        // Start .NET CoreCLR hosting
        StartScriptEngine();

        // Retrieve and call managed HelloWorld
        auto hello = GetFunctionPtr<void(*)(void)>(
            "ScriptAPI",                 // Assembly name
            "ScriptAPI.EngineInterface", // Namespace.ClassName
            "HelloWorld"                 // Method name
        );

        // Execute the managed function
        hello();

        StopScriptEngine();
        CleanUp();
    }

    void Application::StartScriptEngine()
    {
        std::string runtimePath(MAX_PATH, '\0');
        GetModuleFileNameA(nullptr, runtimePath.data(), MAX_PATH);
        PathRemoveFileSpecA(runtimePath.data());
        runtimePath.resize(strlen(runtimePath.data()));

        std::string coreClrPath = runtimePath + "\\coreclr.dll";
        coreClr = LoadLibraryExA(coreClrPath.c_str(), nullptr, 0);
        if (!coreClr)
            throw std::runtime_error("Failed to load coreclr.dll");

        // Get CoreCLR functions
        initializeCoreClr = getCoreClrFuncPtr<coreclr_initialize_ptr>("coreclr_initialize");
        createManagedDelegate = getCoreClrFuncPtr<coreclr_create_delegate_ptr>("coreclr_create_delegate");
        shutdownCoreClr = getCoreClrFuncPtr<coreclr_shutdown_ptr>("coreclr_shutdown");

        // Build TPA list (list of trusted DLLs)
        std::string tpaList = buildTpaList(runtimePath);

        std::array propertyKeys = { "TRUSTED_PLATFORM_ASSEMBLIES", "APP_PATHS" };
        std::array propertyValues = { tpaList.c_str(), runtimePath.c_str() };

        // Initialize CLR
        int result = initializeCoreClr(
            runtimePath.c_str(),
            "CrashoutHost",
            static_cast<int>(propertyKeys.size()),
            propertyKeys.data(),
            propertyValues.data(),
            &hostHandle,
            &domainId
        );

        if (result < 0)
            throw std::runtime_error("Failed to initialize CoreCLR");
    }

    std::string Application::buildTpaList(const std::string& directory)
    {
        static const std::string SEARCH_PATH = directory + "\\*.dll";
        static constexpr char PATH_DELIMITER = ';';

        std::ostringstream tpaList;
        WIN32_FIND_DATAA findData;
        HANDLE fileHandle = FindFirstFileA(SEARCH_PATH.c_str(), &findData);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                tpaList << directory << '\\' << findData.cFileName << PATH_DELIMITER;
            } while (FindNextFileA(fileHandle, &findData));
            FindClose(fileHandle);
        }

        return tpaList.str();
    }

    void Application::StopScriptEngine()
    {
        int result = shutdownCoreClr(hostHandle, domainId);
        if (result < 0)
            throw std::runtime_error("Failed to shut down CoreCLR");
    }
    void Application::HelloWorld()
    {
        std::cout << "Hello from native side!" << std::endl;
    }

}
