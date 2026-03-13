/*!
@file       scriptvalidation.cpp
@author     Renfred Lee
@author     Mohamad Amir Akmal
@author     Ranvitha
@date       26/11/2025

Defines the functions for script validation.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "ScriptValidation.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "engine.h"

namespace CrashoutsEngine
{
    bool IsValidScriptBehaviourClass(const std::string& className)
    {
        namespace fs = std::filesystem;
        fs::path base = fs::path(GetGameRoot()) / "scripts";

        try {
            for (auto it = fs::recursive_directory_iterator(
                base, fs::directory_options::skip_permission_denied);
                it != fs::recursive_directory_iterator(); ++it)
            {
                if (!it->is_regular_file()) continue;

                auto ext = it->path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".cs") continue;

                // Only check files whose name matches className
                if (it->path().stem().string() != className)
                    continue;

                // Open and scan file
                std::ifstream in(it->path());
                if (!in) return false;

                std::string line;
                while (std::getline(in, line)) {
                    if (line.find("[ScriptBehaviour]") != std::string::npos) {
                        return true;
                    }
                }
                // Found file but no attribute
                return false;
            }
        }
        catch (...) {
            // treat as invalid
        }
        return false; // no file found
    }
}