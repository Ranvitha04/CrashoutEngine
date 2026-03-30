/*!
@file       buildanalyzer.h
@author     Renfred Lee
@brief      Header for the Engine Build & Project Management subsystem.
*/
#pragma once

#include "debugmenu.h" // Requires UIState definition
#include <string>

namespace UI {
    /*!
     * @brief Renders the UI panel responsible for compiling and packaging the standalone game.
     * @param ui The global UI state toggle data.
     */
    void RenderBuildPanel(UIState& ui);

    /*!
     * @brief Renders the UI panel for creating new projects and switching between them.
     * @param ui The global UI state toggle data.
     */
    void RenderProjectManager(UIState& ui);

    /*!
     * @brief Modifies the assetmanifest.json to set the default scene to load on boot.
     * @param sceneName The exact name of the scene file (e.g., "Level1.json").
     */
    void SetStartSceneInManifest(const std::string& sceneName);

    /*!
     * @brief Executes a command line instruction invisibly via the Windows API, capturing standard output into our engine logger.
     * @param cmd The string command to run (e.g., "dotnet build").
     * @return The integer exit code from the process (0 is success).
     */
    int ExecuteCommandHidden(const std::string& cmd);
}