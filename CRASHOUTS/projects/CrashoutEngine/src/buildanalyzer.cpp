/*!
@file       buildanalyzer.cpp
@author     Renfred Lee
@brief      Implementation of the Build and Project Management panels.
*/

#include "buildanalyzer.h"
#include "engine.h"
#include "crashlogger.h"
#include "audiocomponent.h"       // For unloading audio on project switch
#include "ScriptSystem.h"         // For reloading scripts on project switch
#include "collidereventmanager.h" // For clearing collisions on project switch
#include "debugmenu_internals.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>

#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>
#include <map>         // <--- NEW: Required to group our assets into folder tabs

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShObjIdl.h>    // Required for modern Windows File Dialogs
#endif

using namespace CrashoutsEngine;

namespace {
    // =========================================================================
    // C++ / CMAKE / C# SCAFFOLDING TEMPLATES
    // =========================================================================

    const std::string MAIN_CPP_TEMPLATE = R"(
// Extension loader library's header must be included before GLFW's header!!!
#pragma warning(push)
#pragma warning(disable: 4005)
#include "engine.h"
#pragma warning(pop)

#define NOMINMAX     
#include <windows.h> 
#include <cstdio>
#include <crtdbg.h>

#include "transformcomponent.h"
#include "performanceViewer.h"
#include "glapp.h"
#include "frameratecontroller.h"
#include "crashlogger.h"

static CrashoutsEngine::Entity globalSelectedEntity = 0;
auto& FRC = FrameRateController::GetInstance();

int WINAPI WinMain(
    [[maybe_unused]] _In_     HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_     LPSTR     lpCmdLine,
    [[maybe_unused]] _In_     int       nCmdShow
) {
#if DEBUG
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    SetupConsole();
#endif

    InstallCrashLogger("CrashoutsEngine");

    // I use preprocessor macros here so the executable knows if it's running inside the Editor or as a standalone game.
#if USE_IMGUI_EDITOR
    SetGameRoot("../../projects/GameProjects/{{GAME_NAME}}");
#else
    SetGameRoot(".");
#endif

    Init();
    double fixedDT = FRC.FixedDT();

    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        FRC.FrameStart();
        glfwPollEvents();
        fixedDT = FRC.FixedDT();
        Update(fixedDT);
        Draw(globalSelectedEntity);
        FRC.FrameEnd();
    }

    LogExitCode();
    CleanUp();
    return EXIT_SUCCESS;
}
)";

    const std::string CMAKELISTS_TEMPLATE = R"(
cmake_minimum_required(VERSION 3.20)
project({{GAME_NAME}})

set(project_dir ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE src_files "${project_dir}/src/*.cpp")
file(GLOB_RECURSE inc_files "${project_dir}/include/*.h")

add_executable(${PROJECT_NAME} WIN32 ${src_files} ${inc_files})
target_link_libraries(${PROJECT_NAME} PRIVATE CrashoutEngine)
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${project_dir}/include" 
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

add_executable(CrashoutEditor WIN32 ${src_files} ${inc_files})
target_link_libraries(CrashoutEditor PRIVATE CrashoutEngine_Editor)
target_include_directories(CrashoutEditor PRIVATE 
    "${project_dir}/include" 
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)
set_target_properties(CrashoutEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrashoutEditor>")

set(CSPROJ_PATH "${project_dir}/scripts/{{GAME_NAME}}.csproj")
add_custom_target({{GAME_NAME}}_Scripts ALL
    COMMAND dotnet build "${CSPROJ_PATH}" -c $<CONFIG>
    WORKING_DIRECTORY "${project_dir}/scripts"
)
add_dependencies(${PROJECT_NAME} {{GAME_NAME}}_Scripts)
add_dependencies(CrashoutEditor {{GAME_NAME}}_Scripts)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/core-assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/core-assets"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/build/x64-$<CONFIG>/scripts/${PROJECT_NAME}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts"
)
)";

    const std::string CSPROJ_TEMPLATE = R"(<Project ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <Configuration Condition=" '$(Configuration)' == '' ">Debug</Configuration>
    <Platform Condition=" '$(Platform)' == '' ">AnyCPU</Platform>
    <ProjectGuid>{A1234567-ABCD-4321-9999-123456789ABC}</ProjectGuid>
    <OutputType>Library</OutputType>
    <RootNamespace>CrashoutsEngine</RootNamespace>
    <AssemblyName>{{GAME_NAME}}</AssemblyName>
    <TargetFrameworkVersion>v4.7.2</TargetFrameworkVersion>
    <LangVersion>latest</LangVersion>
    <Deterministic>false</Deterministic>
    
    <OutputPath>..\..\..\..\build\x64-$(Configuration)\scripts\{{GAME_NAME}}\</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
  </PropertyGroup>

  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Debug|AnyCPU' ">
    <DebugSymbols>true</DebugSymbols>
    <DebugType>full</DebugType>
    <Optimize>false</Optimize>
    <DefineConstants>DEBUG;TRACE</DefineConstants>
  </PropertyGroup>

  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Release|AnyCPU' ">
    <DebugType>pdbonly</DebugType>
    <Optimize>true</Optimize>
    <DefineConstants>TRACE</DefineConstants>
  </PropertyGroup>

  <ItemGroup>
    <Reference Include="System" />
    <Reference Include="System.Core" />
    <Reference Include="mscorlib" />
  </ItemGroup>

  <ItemGroup>
    <Compile Include="**\*.cs" Exclude="obj\**;bin\**" />
    <Compile Include="..\..\..\CrashoutEngine\scripts\**\*.cs" />
  </ItemGroup>

  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
</Project>
)";

    /*!
     * @brief Finds and replaces a placeholder string inside a file, then writes it to disk.
     */
    void ScaffoldProjectFile(const std::filesystem::path& targetPath, std::string content, const std::string& gameName) {
        size_t pos = 0;
        // I dynamically inject the specific game name into our raw string templates before saving.
        while ((pos = content.find("{{GAME_NAME}}", pos)) != std::string::npos) {
            content.replace(pos, 13, gameName);
            pos += gameName.length();
        }
        std::ofstream ofs(targetPath);
        if (ofs.is_open()) {
            ofs << content;
            ofs.close();
        }
    }
} // end anonymous namespace

namespace UI {

    void SetStartSceneInManifest(const std::string& sceneName) {
        std::string manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
        std::ifstream ifs(manifestPath);
        if (!ifs.is_open()) {
            LOGE(Core, "Could not open manifest to set start scene.");
            return;
        }

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        ifs.close();

        if (doc.HasParseError() || !doc.IsObject()) {
            LOGE(Core, "Manifest parse error.");
            return;
        }

        // I either update the existing key or insert a new one if this is a fresh project.
        if (doc.HasMember("start_scene")) {
            doc["start_scene"].SetString(sceneName.c_str(), doc.GetAllocator());
        }
        else {
            rapidjson::Value key("start_scene", doc.GetAllocator());
            rapidjson::Value val(sceneName.c_str(), doc.GetAllocator());
            doc.AddMember(key, val, doc.GetAllocator());
        }

        // Write out the JSON using PrettyWriter so developers can still read it in a text editor.
        std::ofstream ofs(manifestPath);
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
        doc.Accept(writer);

        LOGI(Core, ("Set Start Scene to: " + sceneName).c_str());
    }

    int ExecuteCommandHidden(const std::string& cmd) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hReadPipe = NULL;
        HANDLE hWritePipe = NULL;

        // I establish a memory pipe here so the hidden child process can stream its text output back to the Engine.
        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
            LOGE(Core, "Failed to create output pipe.");
            return -1;
        }

        if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0)) {
            LOGE(Core, "Failed to set handle information.");
            return -1;
        }

        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        STARTUPINFOA siStartInfo;
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdError = hWritePipe;
        siStartInfo.hStdOutput = hWritePipe;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        std::string fullCmd = "cmd.exe /c " + cmd;
        std::vector<char> cmdBuffer(fullCmd.begin(), fullCmd.end());
        cmdBuffer.push_back('\0');

        // CREATE_NO_WINDOW is crucial here. It forces Windows to keep the CMD process entirely invisible.
        BOOL bSuccess = CreateProcessA(
            NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo
        );

        if (!bSuccess) {
            LOGE(Core, "Failed to create hidden process.");
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            return -1;
        }

        // The write handle must be closed in the parent thread, or ReadFile will hang forever waiting for EOF.
        CloseHandle(hWritePipe);

        DWORD dwRead;
        CHAR chBuf[256];
        BOOL bSuccessRead = FALSE;
        std::string currentLine = "";

        // Continually read from the pipe and parse lines until the process finishes.
        for (;;) {
            bSuccessRead = ReadFile(hReadPipe, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
            if (!bSuccessRead || dwRead == 0) break;

            chBuf[dwRead] = '\0';
            for (DWORD i = 0; i < dwRead; ++i) {
                if (chBuf[i] == '\n') {
                    if (!currentLine.empty() && currentLine.back() == '\r') currentLine.pop_back();
                    if (!currentLine.empty()) {
                        std::string lowerLine = currentLine;
                        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

                        // I scan for keywords to automatically assign LOGE, LOGS, or LOGI macro colors.
                        if (lowerLine.find("error") != std::string::npos || lowerLine.find("failed") != std::string::npos) {
                            LOGE(Build, currentLine.c_str());
                        }
                        else if (lowerLine.find("success") != std::string::npos || lowerLine.find("succeeded") != std::string::npos) {
                            LOGS(Build, currentLine.c_str());
                        }
                        else {
                            LOGI(Build, currentLine.c_str());
                        }
                    }
                    currentLine.clear();
                }
                else {
                    currentLine += chBuf[i];
                }
            }
        }

        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(piProcInfo.hProcess, &exitCode);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hReadPipe);

        return (int)exitCode;
#else
        return std::system(cmd.c_str());
#endif
    }

    // =========================================================================
    // BUILD PANEL LOGIC & STATES
    // =========================================================================

    struct ExportAssetInfo {
        std::string relativePath;
        uintmax_t sizeBytes;
        bool isSelected;
    };

    static std::vector<ExportAssetInfo> s_AssetList;
    static uintmax_t s_EssentialsSizeBytes = 0;
    static std::atomic<bool> g_IsBuilding{ false };
    static char g_BuildStatusMsg[256] = "Idle";
    static float g_BuildProgress = 0.0f;
    static std::string g_EstimatedSize = "Calculating...";

    /*!
 * @brief Saves the current build configuration into the shared settings.json file.
 * @note We use a Read-Modify-Write pattern so we don't accidentally overwrite
 * the UI toggles or audio volumes managed by settings.cpp!
 */
    static void SaveBuildSettings(const std::string& projectRoot, const std::string& exportDir, const std::vector<ExportAssetInfo>& assetList) {
        std::string path = projectRoot + "/assets/settings.json";
        rapidjson::Document doc;

        // 1. Read existing settings file first to preserve standard editor states
        std::ifstream ifs(path);
        if (ifs.is_open()) {
            rapidjson::IStreamWrapper isw(ifs);
            doc.ParseStream(isw);
            ifs.close();
        }

        // 2. Ensure it's a valid JSON object
        if (!doc.IsObject()) {
            doc.SetObject();
        }

        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        // 3. Update or Add Export Path
        if (doc.HasMember("ExportPath")) {
            doc["ExportPath"].SetString(exportDir.c_str(), allocator);
        }
        else {
            doc.AddMember("ExportPath", rapidjson::Value(exportDir.c_str(), allocator), allocator);
        }

        // 4. Update or Add Deselected Assets Array
        rapidjson::Value deselectedArray(rapidjson::kArrayType);
        for (const auto& asset : assetList) {
            if (!asset.isSelected) {
                deselectedArray.PushBack(rapidjson::Value(asset.relativePath.c_str(), allocator), allocator);
            }
        }

        if (doc.HasMember("DeselectedAssets")) {
            doc["DeselectedAssets"] = deselectedArray;
        }
        else {
            doc.AddMember("DeselectedAssets", deselectedArray, allocator);
        }

        // 5. Write everything back to disk
        std::ofstream ofs(path);
        if (!ofs.is_open()) {
            LOGE(Core, "Failed to save build configuration to settings.json");
            return;
        }

        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
        doc.Accept(writer);
    }

    /*!
     * @brief Loads the build configuration from the shared settings.json file.
     */
    static void LoadBuildSettings(const std::string& projectRoot, char* exportDirBuf, size_t exportDirBufSize, std::vector<ExportAssetInfo>& assetList) {
        std::string path = projectRoot + "/assets/settings.json";
        if (!std::filesystem::exists(path)) return;

        std::ifstream ifs(path);
        if (!ifs.is_open()) return;

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        ifs.close();

        if (doc.HasParseError() || !doc.IsObject()) return;

        // Restore the target export folder
        if (doc.HasMember("ExportPath") && doc["ExportPath"].IsString()) {
            std::string savedPath = doc["ExportPath"].GetString();
            if (!savedPath.empty()) {
                strncpy(exportDirBuf, savedPath.c_str(), exportDirBufSize - 1);
                exportDirBuf[exportDirBufSize - 1] = '\0';
            }
        }

        // Restore the unchecked assets via instant unordered_set lookup
        if (doc.HasMember("DeselectedAssets") && doc["DeselectedAssets"].IsArray()) {
            std::unordered_set<std::string> deselectedSet;
            for (const auto& item : doc["DeselectedAssets"].GetArray()) {
                if (item.IsString()) {
                    deselectedSet.insert(item.GetString());
                }
            }

            for (auto& asset : assetList) {
                if (deselectedSet.find(asset.relativePath) != deselectedSet.end()) {
                    asset.isSelected = false;
                }
            }
        }
    }

    /*!
     * @brief Formats byte counts into human-readable KB or MB strings.
     */
    static std::string FormatBytes(uintmax_t bytes) {
        if (bytes < 1024) return std::to_string(bytes) + " B";
        else if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
            return std::string(buf);
        }
    }

    /*!
     * @brief Recursively iterates through a directory to sum the file sizes.
     */
    static uintmax_t CalculateDirectorySize(const std::filesystem::path& dir) {
        uintmax_t size = 0;
        if (std::filesystem::exists(dir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::error_code ec; // Prevent throwing an exception if a file is OS-locked
                    size += entry.file_size(ec);
                }
            }
        }
        return size;
    }

    /*!
     * @brief Triggers the native Windows Common Item Dialog to select a folder path.
     */
    static std::string OpenFolderDialog() {
        std::string result = "";
#ifdef _WIN32
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        IFileDialog* pfd;

        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
            DWORD dwOptions;
            if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
                pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM); // Restrict to folders only
            }
            if (SUCCEEDED(pfd->Show(NULL))) {
                IShellItem* psi;
                if (SUCCEEDED(pfd->GetResult(&psi))) {
                    PWSTR pszPath;
                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                        // I must convert the Windows WideString (UTF-16) to standard UTF-8 for our C++ engine.
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, NULL, 0, NULL, NULL);
                        std::string str(size_needed, 0);
                        WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &str[0], size_needed, NULL, NULL);
                        if (!str.empty() && str.back() == '\0') str.pop_back();
                        result = str;
                        CoTaskMemFree(pszPath);
                    }
                    psi->Release();
                }
            }
            pfd->Release();
        }
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
#endif
        return result;
    }

    void RenderBuildPanel(UIState& ui) {
        ScopedWindow w("Build & Publish", LockFlags(ui.lockLayout));
        namespace fs = std::filesystem;

        std::string currentRoot = GetGameRoot();
        std::string gameName = fs::path(currentRoot).filename().string();

        ImGui::Text("Active Project:");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gameName.c_str());
        ImGui::Separator();

        static char exportPath[256] = { 0 };
        static std::string lastGameName = "";

        // I only calculate the massive asset lists when the UI first loads or switches games to save performance.
        if (lastGameName != gameName) {
            fs::path defaultExportPath = fs::path(currentRoot).parent_path().parent_path().parent_path() / "projects/ExportedGames";
            std::string pathStr = defaultExportPath.generic_string() + "/";
            strncpy(exportPath, pathStr.c_str(), sizeof(exportPath) - 1);
            exportPath[sizeof(exportPath) - 1] = '\0';

            std::string workspaceRoot = fs::path(currentRoot).parent_path().parent_path().parent_path().generic_string();

            // 1. Gather all game-specific assets into a dynamic list for the checkbox UI
            s_AssetList.clear();
            fs::path assetsPath = fs::path(currentRoot) / "assets";
            if (fs::exists(assetsPath)) {
                for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
                    if (entry.is_regular_file()) {
                        ExportAssetInfo info;
                        info.relativePath = fs::relative(entry.path(), assetsPath).generic_string();
                        std::error_code ec;
                        info.sizeBytes = entry.file_size(ec);
                        info.isSelected = true;
                        s_AssetList.push_back(info);
                    }
                }
            }

            // Load Custom Settings from Disk to Override Defaults
            LoadBuildSettings(currentRoot, exportPath, sizeof(exportPath), s_AssetList);

            // 2. Pre-calculate the engine's core dependencies to provide a baseline file size.
            s_EssentialsSizeBytes = 0;
            fs::path coreAssetsPath = fs::path(workspaceRoot) / "projects/CrashoutEngine/core-assets";
            s_EssentialsSizeBytes += CalculateDirectorySize(coreAssetsPath);

#ifdef _DEBUG
            fs::path sourceBinaries = fs::path(workspaceRoot) / "build" / "x64-Debug";
#else
            fs::path sourceBinaries = fs::path(workspaceRoot) / "build" / "x64-Release";
#endif

            if (fs::exists(sourceBinaries)) {
                s_EssentialsSizeBytes += CalculateDirectorySize(sourceBinaries / "scripts");
                s_EssentialsSizeBytes += CalculateDirectorySize(sourceBinaries / "third_party");

                std::error_code ec;
                if (fs::exists(sourceBinaries / "curl-ca-bundle.crt")) {
                    s_EssentialsSizeBytes += fs::file_size(sourceBinaries / "curl-ca-bundle.crt", ec);
                }

                // Strictly accumulate only the executable and required runtime DLLs.
                for (const auto& entry : fs::directory_iterator(sourceBinaries, ec)) {
                    if (entry.is_regular_file()) {
                        std::string fileName = entry.path().filename().string();
                        std::string ext = entry.path().extension().string();
                        if (ext == ".dll" || fileName == (gameName + ".exe")) {
                            s_EssentialsSizeBytes += entry.file_size(ec);
                        }
                    }
                }
            }
            else {
                s_EssentialsSizeBytes += 600 * 1024 * 1024; // Fallback estimate if not built yet
            }

            lastGameName = gameName;
        }

        ImGui::Text("Export Directory");
        ImGui::SameLine();

        // I manually calculate the required width of the suffix elements so the InputText resizes flawlessly.
        float btnWidth = ImGui::CalcTextSize("...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float helpWidth = ImGui::CalcTextSize("(?)").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float rightReserve = btnWidth + helpWidth + (spacing * 2.0f);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - rightReserve);
        ImGui::InputText("##ExportPathInput", exportPath, IM_ARRAYSIZE(exportPath));

        ImGui::SameLine();
        if (ImGui::Button("...")) {
            std::string chosenFolder = OpenFolderDialog();
            if (!chosenFolder.empty()) {
                std::replace(chosenFolder.begin(), chosenFolder.end(), '\\', '/');
                if (chosenFolder.back() != '/') chosenFolder += '/';
                strncpy(exportPath, chosenFolder.c_str(), sizeof(exportPath) - 1);
                exportPath[sizeof(exportPath) - 1] = '\0';

                // Save immediately if they change the folder manually
                SaveBuildSettings(currentRoot, exportPath, s_AssetList);
            }
        }
        ImGui::SameLine();
        Help("The base folder where your final, playable game will be shipped.");

        // ==========================================================
        // UI: ASSET LIST VIEWER WITH TABS AND WRAPPING
        // ==========================================================
        std::string manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
        std::string startSceneFilename = "";
        std::ifstream ifs(manifestPath);
        if (ifs.is_open()) {
            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document doc;
            doc.ParseStream(isw);
            ifs.close();
            if (!doc.HasParseError() && doc.HasMember("start_scene") && doc["start_scene"].IsString()) {
                startSceneFilename = doc["start_scene"].GetString();
            }
        }

        ImGui::SeparatorText("Game Assets to Package");
        if (ImGui::Button("Select All")) {
            for (auto& a : s_AssetList) a.isSelected = true;
            SaveBuildSettings(currentRoot, exportPath, s_AssetList);
        }
        ImGui::SameLine();
        if (ImGui::Button("Deselect All")) {
            for (auto& a : s_AssetList) {
                // Explicitly prevent "Deselect All" from unchecking the critical Start Scene
                if (std::filesystem::path(a.relativePath).filename().generic_string() != startSceneFilename) {
                    a.isSelected = false;
                }
            }
            SaveBuildSettings(currentRoot, exportPath, s_AssetList);
        }

        // Group the assets by their parent folder into a map
        std::map<std::string, std::vector<ExportAssetInfo*>> groupedAssets;
        for (auto& asset : s_AssetList) {
            fs::path p(asset.relativePath);
            std::string folder = p.parent_path().generic_string();
            if (folder.empty()) folder = "/ (Root)";
            groupedAssets[folder].push_back(&asset);
        }

        // I force the start scene to ALWAYS be selected before calculating bytes, just in case
        for (auto& asset : s_AssetList) {
            if (std::filesystem::path(asset.relativePath).filename().generic_string() == startSceneFilename) {
                asset.isSelected = true;
            }
        }

        uintmax_t selectedAssetsBytes = 0;
        for (auto& asset : s_AssetList) {
            if (asset.isSelected) selectedAssetsBytes += asset.sizeBytes;
        }

        // Dynamic stretching for the asset list window
        ImGui::BeginChild("AssetListRegion", ImVec2(0, ImGui::GetContentRegionAvail().y - 140), true);

        for (auto& [folder, assets] : groupedAssets) {
            if (ImGui::TreeNodeEx(folder.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
                for (auto* asset : assets) {
                    fs::path p(asset->relativePath);
                    std::string fileName = p.filename().generic_string();

                    // Check if this specific asset is our Start Scene 
                    bool isStartScene = (fileName == startSceneFilename);

                    // Gray out the checkbox if it's the start scene so the user can't click it
                    if (isStartScene) ImGui::BeginDisabled();
                    if (ImGui::Checkbox(("##" + asset->relativePath).c_str(), &asset->isSelected)) {
                        // Save immediately when a checkbox is toggled 
                        SaveBuildSettings(currentRoot, exportPath, s_AssetList);
                    }
                    if (isStartScene) ImGui::EndDisabled();

                    ImGui::SameLine();

                    float sizeTextWidth = 75.0f;

                    ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - sizeTextWidth - 20.0f);
                    ImGui::TextWrapped("%s", fileName.c_str());
                    ImGui::PopTextWrapPos();

                    // --- NEW: Add a visual indicator to explain the locked checkbox ---
                    if (isStartScene) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "[Required Start Scene]");
                    }

                    // Pin the file size explicitly to the far right edge of the window
                    ImGui::SameLine(ImGui::GetWindowWidth() - sizeTextWidth);
                    ImGui::TextDisabled("%s", FormatBytes(asset->sizeBytes).c_str());
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        uintmax_t totalEstimatedBytes = s_EssentialsSizeBytes + selectedAssetsBytes;

        ImGui::Text("Estimated Build Size Breakdown:");
        ImGui::BulletText("Essentials (Engine, DLLs, Core): %s", FormatBytes(s_EssentialsSizeBytes).c_str());
        ImGui::BulletText("Selected Game Assets: %s", FormatBytes(selectedAssetsBytes).c_str());
        ImGui::Text("Total Estimated Size: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%s", FormatBytes(totalEstimatedBytes).c_str());

        ImGui::Dummy(ImVec2(0, 10));

        if (g_IsBuilding) {
            ImGui::BeginDisabled();
            ImGui::Button("Building... Please wait", ImVec2(-1, 30));
            ImGui::EndDisabled();
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("Compile & Export Standalone", ImVec2(-1, 30))) {

                // Auto-save settings the moment they hit build just to be 100% safe 
                SaveBuildSettings(currentRoot, exportPath, s_AssetList);

                g_IsBuilding = true;
                g_BuildProgress = 0.1f;

#ifdef _DEBUG
                std::string buildConfig = "Debug";
#else
                std::string buildConfig = "Release";
#endif

                snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Initializing %s build for %s...", buildConfig.c_str(), gameName.c_str());
                LOGI(Build, g_BuildStatusMsg);

                std::string finalExportDir = std::string(exportPath) + gameName;

                // I must copy the asset states to a local vector so the background thread can safely read them without race conditions.
                std::vector<std::string> assetsToCopy;
                for (const auto& a : s_AssetList) {
                    if (a.isSelected) assetsToCopy.push_back(a.relativePath);
                }

                // Detached thread prevents the editor UI from hanging while compiling!
                std::thread([gameName, currentRoot, finalExportDir, buildConfig, assetsToCopy]() {

                    std::string workspaceRoot = fs::path(currentRoot).parent_path().parent_path().parent_path().generic_string();
                    std::string mainBuildDir = workspaceRoot + "/build";

                    // --- STEP 1: Build C# ---
                    snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Compiling C# Scripts (%s)...", buildConfig.c_str());
                    LOGI(Build, g_BuildStatusMsg);
                    g_BuildProgress = 0.3f;

                    std::string csProj = currentRoot + "/scripts/" + gameName + ".csproj";
                    std::string buildCS = "dotnet build \"" + csProj + "\" -c " + buildConfig;
                    int csRes = ExecuteCommandHidden(buildCS);
                    if (csRes != 0) {
                        LOGE(Build, "C# Compilation Failed! See logs above.");
                    }

                    // --- STEP 2: Configure CMake ---
                    snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Configuring CMake for %s...", gameName.c_str());
                    LOGI(Build, g_BuildStatusMsg);
                    g_BuildProgress = 0.4f;

                    std::string configCPP = "cmake -S \"" + workspaceRoot + "\" -B \"" + mainBuildDir + "\" -DGAME_TARGET=" + gameName;
                    int configRes = ExecuteCommandHidden(configCPP);

                    if (configRes != 0) {
                        snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "CMake Configure Failed! Check the console.");
                        LOGE(Build, g_BuildStatusMsg);
                        g_IsBuilding = false;
                        return;
                    }

                    // --- STEP 3: Compile C++ Game ---
                    snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Compiling C++ Engine & Game (%s)...", buildConfig.c_str());
                    LOGI(Build, g_BuildStatusMsg);
                    g_BuildProgress = 0.6f;

                    std::string buildCPP = "cmake --build \"" + mainBuildDir + "\" --config " + buildConfig + " --target " + gameName;
                    int cppRes = ExecuteCommandHidden(buildCPP);

                    if (cppRes != 0) {
                        snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Build Failed! Check the console window for MSBuild errors.");
                        LOGE(Build, g_BuildStatusMsg);
                        g_IsBuilding = false;
                        return;
                    }

                    // --- STEP 4: Package ---
                    snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Packaging Selected Assets & Executable...");
                    LOGI(Build, g_BuildStatusMsg);
                    g_BuildProgress = 0.8f;

                    std::error_code ec;
                    if (fs::exists(finalExportDir)) {
                        snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Clearing previous export...");
                        LOGI(Build, g_BuildStatusMsg);
                        fs::remove_all(finalExportDir, ec);
                    }

                    fs::create_directories(finalExportDir, ec);
                    fs::path sourceBinaries = fs::path(mainBuildDir) / ("x64-" + buildConfig);

                    if (fs::exists(sourceBinaries)) {

                        auto copy_item = [&](const std::string& item) {
                            fs::path src = sourceBinaries / item;
                            fs::path dst = fs::path(finalExportDir) / item;
                            std::error_code c_ec;
                            if (fs::exists(src)) {
                                if (fs::is_directory(src)) {
                                    fs::create_directories(dst, c_ec);
                                    fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, c_ec);
                                }
                                else {
                                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, c_ec);
                                }
                            }
                            };

                        copy_item(gameName + ".exe");
                        copy_item("core-assets");
                        copy_item("scripts");
                        copy_item("third_party");
                        copy_item("curl-ca-bundle.crt");

                        for (const auto& entry : fs::directory_iterator(sourceBinaries, ec)) {
                            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                                copy_item(entry.path().filename().string());
                            }
                        }

                        // Selectively loop only over the assets the user checked in the UI!
                        fs::path srcAssets = fs::path(currentRoot) / "assets";
                        fs::path dstAssets = fs::path(finalExportDir) / "assets";
                        fs::create_directories(dstAssets, ec);

                        for (const auto& relPath : assetsToCopy) {
                            fs::path srcFile = srcAssets / relPath;
                            fs::path dstFile = dstAssets / relPath;
                            fs::create_directories(dstFile.parent_path(), ec);
                            fs::copy_file(srcFile, dstFile, fs::copy_options::overwrite_existing, ec);
                        }

                        snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Success! Game exported to: %s", finalExportDir.c_str());
                        LOGS(Build, g_BuildStatusMsg);
                        g_BuildProgress = 1.0f;
                    }
                    else {
                        snprintf(g_BuildStatusMsg, sizeof(g_BuildStatusMsg), "Error: Could not find CMake output at %s", sourceBinaries.generic_string().c_str());
                        LOGE(Build, g_BuildStatusMsg);
                    }

                    g_IsBuilding = false;

                    }).detach();
            }
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::ProgressBar(g_BuildProgress, ImVec2(-1, 0));

        ImVec4 statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (std::string(g_BuildStatusMsg).find("Success") != std::string::npos) statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        else if (std::string(g_BuildStatusMsg).find("Failed") != std::string::npos || std::string(g_BuildStatusMsg).find("Error") != std::string::npos) statusColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        else if (g_IsBuilding) statusColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);

        // I force ImGui to wrap text to the panel width so long success messages don't vanish off screen.
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(statusColor, "Status: %s", g_BuildStatusMsg);
        ImGui::PopTextWrapPos();
    }

    void RenderProjectManager(UIState& ui) {
        ScopedWindow w("Project Manager", LockFlags(ui.lockLayout));
        namespace fs = std::filesystem;

        ImGui::Text("Current Project Root:");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", GetGameRoot().c_str());

        ImGui::Separator();

        auto SwitchProjectContext = [](const std::string& projName, const std::string& projPath) {
            Audio::AudioSystem& audio = Audio::AudioSystem::instance();
            audio.stopAll(true);
            audio.unloadAllBanks();
            audio.unloadAllRawSounds();

            CrashoutsEngine::ScriptSystem::DestroyAll();
            CrashoutsEngine::ECS::getInstance()->Reset();
            CrashoutsEngine::CollisionEventManager::GetInstance().Clear();

            SetGameRoot(projPath);

            fs::path csprojPath = fs::path(projPath) / "scripts" / (projName + ".csproj");

            std::string config = "Release";
#ifdef _DEBUG
            config = "Debug";
#endif

            std::string buildCommand = "dotnet build \"" + csprojPath.generic_string() + "\" -c " + config;

            LOGI(Build, ("Compiling scripts for " + projName + " in " + config + " mode...").c_str());
            int buildResult = ExecuteCommandHidden(buildCommand);

            if (buildResult == 0) {
                LOGS(Build, "Script compilation successful!");
            }
            else {
                LOGE(Build, "Script compilation failed. Check your C# code in the console!");
            }

            MonoManager::ReloadAssembly();
            CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

            auto& am = CrashoutsEngine::Assets::AssetManager::GetInstance();
            am.Shutdown();

            std::string newMPath = GetGameRoot() + "/assets/json/assetmanifest.json";
            CrashoutsEngine::Assets::GenerateAssetManifest(newMPath);
            am.LoadManifestAssets(newMPath);

            std::string newPrefabPath = GetGameRoot() + "/assets/prefabs";
            CrashoutsEngine::ReloadPrefabsFromDisk(newPrefabPath);

            g_PendingSceneName = GetStartingSceneName();
            g_IsSceneLoadPending = true;

            LOGS(Core, ("Context Switched to: " + projName + " | Booting into: " + g_PendingSceneName).c_str());
            };

        ImGui::SeparatorText("Create New Project");

        static char newProjectName[64] = "";
        ImGui::InputTextWithHint("##ProjName", "Project Name", newProjectName, IM_ARRAYSIZE(newProjectName));

        if (ImGui::Button("Create & Initialize Project")) {
            if (newProjectName[0] != '\0') {
                std::string gameName = newProjectName;
                fs::path projectPath = fs::path("..") / ".." / "projects" / "GameProjects" / gameName;
                fs::path templatePath = fs::path("..") / ".." / "projects" / "Template";

                try {
                    fs::create_directories(projectPath / "assets/images");
                    fs::create_directories(projectPath / "assets/scenes");
                    fs::create_directories(projectPath / "assets/json");
                    fs::create_directories(projectPath / "src");
                    fs::create_directories(projectPath / "include");
                    fs::create_directories(projectPath / "scripts");

                    std::ofstream dummyScene(projectPath / "assets/scenes/StartScene.json");
                    if (dummyScene.is_open()) {
                        dummyScene << "{\n  \"Entities\": []\n}";
                        dummyScene.close();
                    }

                    ScaffoldProjectFile(projectPath / "CMakeLists.txt", CMAKELISTS_TEMPLATE, gameName);
                    ScaffoldProjectFile(projectPath / "src/main.cpp", MAIN_CPP_TEMPLATE, gameName);
                    ScaffoldProjectFile(projectPath / ("scripts/" + gameName + ".csproj"), CSPROJ_TEMPLATE, gameName);

                    if (fs::exists(templatePath / "scripts")) {
                        fs::copy(templatePath / "scripts", projectPath / "scripts",
                            fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                    }

                    LOGS(Core, ("Project " + gameName + " is ready!").c_str());

                    SwitchProjectContext(gameName, projectPath.generic_string());

                    newProjectName[0] = '\0';
                }
                catch (const std::exception& e) {
                    LOGE(Core, (std::string("Scaffolding failed: ") + e.what()).c_str());
                }
            }
        }

        ImGui::SeparatorText("Project Quick Switch");

        fs::path gamesDir = fs::path("..") / ".." / "projects" / "GameProjects";
        if (fs::exists(gamesDir)) {
            for (auto const& entry : fs::directory_iterator(gamesDir)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    std::string currentRoot = GetGameRoot();
                    bool isActive = (currentRoot.find(name) != std::string::npos);

                    if (ImGui::Selectable(name.c_str(), isActive)) {
                        if (isActive) {
                            LOGI(Core, ("Project '" + name + "' is already active.").c_str());
                        }
                        else {
                            SwitchProjectContext(name, entry.path().generic_string());
                        }
                    }
                }
            }
        }
    }
}