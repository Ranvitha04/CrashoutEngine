#pragma once
#include "engine.h"
#include "ImportExport.h"
#include "../include/coreclrhost.h"

namespace Core
{
    class DLL_API Application
    {
    public:
        void Run();
        static void HelloWorld();

    private:
        void StartScriptEngine();
        void StopScriptEngine();
        std::string buildTpaList(const std::string& directory);

        // CoreCLR handles
        HMODULE coreClr = nullptr;
        void* hostHandle = nullptr;
        unsigned int domainId = 0;

        // CoreCLR function pointers
        coreclr_initialize_ptr      initializeCoreClr = nullptr;
        coreclr_create_delegate_ptr createManagedDelegate = nullptr;
        coreclr_shutdown_ptr        shutdownCoreClr = nullptr;

        template<typename T>
        T getCoreClrFuncPtr(const std::string& name)
        {
            auto ptr = reinterpret_cast<T>(GetProcAddress(coreClr, name.c_str()));
            if (!ptr)
                throw std::runtime_error("Failed to get function: " + name);
            return ptr;
        }

        template<typename FuncType>
        FuncType GetFunctionPtr(const std::string_view& assemblyName,
            const std::string_view& typeName,
            const std::string_view& functionName)
        {
            FuncType func = nullptr;
            int result = createManagedDelegate(
                hostHandle, domainId,
                assemblyName.data(),
                typeName.data(),
                functionName.data(),
                reinterpret_cast<void**>(&func)
            );

            if (result < 0)
                throw std::runtime_error("Failed to get managed delegate");
            return func;
        }
    };
}
