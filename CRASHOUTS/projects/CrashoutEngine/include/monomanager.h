/*!
@file		monomanager.h
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Declaration of Mono Manager class

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <filesystem>
#include <string>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
//#include <mono/metadata/appdomain.h>

namespace CrashoutsEngine::Bindings {
	void RegisterDebugBindings();
};

class MonoManager
{
public:
	static bool s_IsManualReloading;
	static void Init(const std::string& domainName = "CrashoutDomain");
	static void Shutdown();
	static void LoadAssembly();

	// Check for changes to DLL and reload if necessary
	static void CheckHotReload();

	// Manually force script reload
	static void ReloadScripts();
	static void ReloadAssembly();

	static MonoDomain* GetDomain() { return s_Domain; }
	static MonoAssembly* GetAssembly() { return s_Assembly; }
	static MonoImage* GetImage();
	static std::string CopyAssemblyToTemp(const std::string& originalPath);

	static void SetLastWriteTime(std::filesystem::file_time_type t) {
		s_LastWriteTime = t;
	}

	static const std::string GetAssemblyPath() {
		return s_AssemblyPath;
	}

	static MonoClass* GetSerializeFieldClass();

private:
	static MonoDomain* s_Domain;
	static MonoAssembly* s_Assembly; // store last loaded assembly
	// For hot reload tracking
	static std::filesystem::file_time_type s_LastWriteTime;
	static std::string s_AssemblyPath;
};
