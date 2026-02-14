/*!
@file		monomanager.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of mono manager functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "monomanager.h"
#include "engine.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <mono/metadata/mono-config.h>
#include "entitymanager.h"
#include "ScriptSystem.h"
#include <thread>

#include "mono/metadata/object.h"
#include "mono/metadata/loader.h"
#include "log.h" 
#include "rendering.h"
#include "shapes.h"
#include "debugmenu.h"

MonoDomain* MonoManager::s_Domain = nullptr;
MonoAssembly* MonoManager::s_Assembly = nullptr;
std::filesystem::file_time_type MonoManager::s_LastWriteTime{};
std::string MonoManager::s_AssemblyPath = "";
bool MonoManager::s_IsManualReloading = false;

MonoClass* MonoManager::GetSerializeFieldClass() {
	if (s_Assembly) {
		MonoImage* image = mono_assembly_get_image(s_Assembly);
		return mono_class_from_name(image, "CrashoutsEngine", "SerializeField");
	}
	return nullptr;
}

namespace CrashoutsEngine::Bindings {

	// 1. The C++ Helper Function
	static void InternalLog(MonoString* message) {
		if (message == nullptr) return;

		// Convert C# string to C++ string
		char* cStr = mono_string_to_utf8(message);
		std::string msg(cStr);
		mono_free(cStr);

		// Send to your ImGui Console
		Logger::Instance().Add(LogMessage::Level::Info, LogMessage::Subsystem::Scripting, msg);
	}

	static void InternalDrawLine(CEVec3* start, CEVec3* end, float thickness, CEVec3* color) {
		if (!UI::IsGameDebugDrawEnabled()) return;
		if (!start || !end || !color) return;
		CEColor col(color->x, color->y, color->z, 1.0f);
		// Queue the line to be drawn at the end of the frame (Post-Clear)
		Component::Rendering::Get().QueueLine(*start, *end, thickness, col, true);
	}

	static void InternalDrawTrajectory(CEVec3* start, CEVec3* end, float thickness, CEVec3* color) {
		if (!start || !end || !color) return;
		CEColor col(color->x, color->y, color->z, 1.0f);
		Component::Rendering::Get().QueueLine(*start, *end, thickness, col, true);
	}

	static void InternalDrawCircle(CEVec2* size, CEVec3* position, CEVec3* color) {
		if (!UI::IsGameDebugDrawEnabled()) return;
		if (!size || !position || !color) return;
		CEColor col(color->x, color->y, color->z, 1.0f);
		// Queue the circle to be drawn at the end of the frame
		Component::Rendering::Get().QueueCircle(*size, *position, col, true);
	}

	// 2. Register it (Call this where you register other bindings)
	void RegisterDebugBindings() {
		mono_add_internal_call("CrashoutsEngine.Debug::Log", (const void*)InternalLog);
		mono_add_internal_call("CrashoutsEngine.Debug::DrawLine", (const void*)InternalDrawLine);
		mono_add_internal_call("CrashoutsEngine.Debug::DrawTrajectory", (const void*)InternalDrawTrajectory);
		mono_add_internal_call("CrashoutsEngine.Debug::DrawCircle", (const void*)InternalDrawCircle);
	}
};


void MonoManager::Init(const std::string& domainName)
{
	mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");
	mono_config_parse(nullptr);

	s_Domain = mono_jit_init(domainName.c_str());

	if (!s_Domain)
	{
		std::cerr << "[Mono] Failed to initialize JIT.\n";
	}
	else
	{
		std::cout << "[Mono] JIT initialized successfully.\n";
	}

	// Only try to read timestamp if the DLL path exists
	namespace fs = std::filesystem;
	if (!s_AssemblyPath.empty() && fs::exists(s_AssemblyPath))
	{
		s_LastWriteTime = fs::last_write_time(s_AssemblyPath);
	}
	else
	{
		std::cerr << "[Mono][Info] Skipping timestamp tracking assembly not loaded yet.\n";
		s_LastWriteTime = fs::file_time_type::clock::now(); // placeholder
	}
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
	(void)assemblyPath;
	namespace fs = std::filesystem;

	// Find the newest DLL in scripts/
	fs::path latestDll;
	auto latestTime = fs::file_time_type::min();

	for (const auto& file : fs::directory_iterator("scripts"))
	{
		if (file.path().extension() == ".dll")
		{
			auto t = fs::last_write_time(file.path());
			if (t > latestTime)
			{
				latestTime = t;
				latestDll = file.path();
			}
		}
	}

	if (latestDll.empty())
	{
		std::cerr << "[Mono] No ManagedScripts DLL found in scripts/!\n";
		return;
	}

	s_AssemblyPath = latestDll.string();

#if !USE_IMGUI_EDITOR
	// Copy assembly to appdata cache
	std::string assemblyToLoadPath = CopyAssemblyToTemp(s_AssemblyPath);
#else
	std::string assemblyToLoadPath = s_AssemblyPath;
#endif

	if (!std::filesystem::exists(assemblyToLoadPath))
	{
		std::cerr << "[Mono] Assembly not found: " << assemblyToLoadPath << '\n';
		return;
	}

	if (!s_Domain)
	{
		std::cerr << "[Mono] Domain not initialized!\n";
		return;
	}

	s_Assembly = mono_domain_assembly_open(s_Domain, assemblyToLoadPath.c_str());
	if (!s_Assembly)
	{
		std::cerr << "[Mono] Failed to load assembly: " << assemblyToLoadPath << '\n';
		return;
	}

	std::cout << "[Mono] Assembly opened successfully.\n";

	CrashoutsEngine::ScriptSystem::RegisterAllBindings();

	if (!s_AssemblyPath.empty() && fs::exists(s_AssemblyPath))
	{
		s_LastWriteTime = fs::last_write_time(s_AssemblyPath);
	}
	else
	{
		std::cerr << "[Mono][Warning] Cannot track last write time bcos invalid path: " << s_AssemblyPath << "\n";
	}
}

void MonoManager::ReloadAssembly()
{
#if USE_IMGUI_EDITOR
	std::cout << "[Mono] Reloading assembly...\n";

	CrashoutsEngine::ScriptSystem::DestroyAll();

	s_Assembly = nullptr;

	namespace fs = std::filesystem;

	try
	{
		// find newest DLL in scripts folder
		fs::path latestDll;
		auto latestTime = fs::file_time_type::min();
		for (const auto& file : fs::directory_iterator("scripts"))
		{
			if (file.path().extension() == ".dll")
			{
				auto t = fs::last_write_time(file.path());
				if (t > latestTime)
				{
					latestTime = t;
					latestDll = file.path();
				}
			}
		}

		if (latestDll.empty())
		{
			std::cerr << "[Mono] No ManagedScripts DLL found in scripts/!\n";
			return;
		}

		std::cout << "[Mono][Debug] Latest DLL detected: " << latestDll << "\n";

		// prepare cache folder
		fs::path dstDir = "scripts/_cache";
		fs::create_directories(dstDir);

		// generate a unique temp filename
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()).count();
		fs::path dst = dstDir / ("ManagedScripts_reload_" + std::to_string(ms) + ".dll");

		// copy newest DLL into cache
		fs::copy_file(latestDll, dst, fs::copy_options::overwrite_existing);

		// load that DLL
		std::cout << "[Mono][Debug] Attempting to load assembly from: " << dst << "\n";
		s_Assembly = mono_domain_assembly_open(s_Domain, dst.string().c_str());
		if (!s_Assembly)
		{
			std::cerr << "[Mono] Reload failed to open assembly!\n";
			return;
		}

		MonoImage* image = mono_assembly_get_image(s_Assembly);
		std::cout << "[Mono][Debug] Assembly loaded successfully. MonoImage: " << image << "\n";
	}
	catch (const fs::filesystem_error& e)
	{
		std::cerr << "[Mono] File operation failed: " << e.what() << "\n";
		return;
	}

	CrashoutsEngine::ScriptSystem::RegisterAllBindings();

	std::cout << "[Mono] Assembly reload completed.\n";
#endif
}

void MonoManager::ReloadScripts()
{
#if USE_IMGUI_EDITOR
	CrashoutsEngine::ScriptSystem::ReloadAll();
#endif
}


void MonoManager::CheckHotReload()
{
#if USE_IMGUI_EDITOR
	if (s_IsManualReloading) return;

	namespace fs = std::filesystem;

	try
	{
		if (!fs::exists(s_AssemblyPath))
			return; // no DLL yet then just skip bruh

		auto currentTime = fs::last_write_time(s_AssemblyPath);
		if (currentTime != s_LastWriteTime)
		{
			s_LastWriteTime = currentTime;
			std::this_thread::sleep_for(std::chrono::milliseconds(300)); // small FS settle delay
			ReloadScripts();
		}
	}
	catch (const fs::filesystem_error& e)
	{
		std::cerr << "[Mono] Hot reload check failed: " << e.what() << '\n';
	}
#endif
}


void MonoManager::Shutdown()
{
	/*
	if (s_Domain)
	{
		mono_jit_cleanup(s_Domain);
		std::cout << "[Mono] Runtime cleaned up\n";
	}
	*/

}

MonoImage* MonoManager::GetImage()
{
	if (!s_Assembly)
		return nullptr;
	return mono_assembly_get_image(s_Assembly);
}

std::string MonoManager::CopyAssemblyToTemp(const std::string& originalPath)
{
	namespace fs = std::filesystem;
	fs::path src = originalPath;

#if !USE_IMGUI_EDITOR
	std::string appDataDir = GetLocalAppData();
	if (appDataDir.empty()) {
		std::cerr << "[Mono] Failed to get LocalAppData path.\n";
		return "";
	}

	fs::path dst = fs::path(appDataDir) / "scripts/_cache";

	// Create directory
	std::error_code ec;
	fs::create_directories(dst, ec);
	if (ec) {
		std::cerr << "[Mono] Failed to create cache directory: " << ec.message();
		return "";
	}

	// Copy DLL to appdata cache
	fs::path tempFile = dst / src.filename();
	fs::copy_file(src, tempFile, fs::copy_options::overwrite_existing);

	return tempFile.string();

#else

	fs::path dst = "scripts/_cache";
	fs::create_directories(dst);

	fs::path tempFile = dst / src.filename();
	fs::copy_file(src, tempFile, fs::copy_options::overwrite_existing);

	return tempFile.string();
#endif
}
