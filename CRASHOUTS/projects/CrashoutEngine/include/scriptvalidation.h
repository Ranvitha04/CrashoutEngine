/* !
@file       scriptvalidation.h
@author     Amir
@date       26/11/2025

Declares the function to validate if a given class name corresponds to a valid script behaviour class.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once
#include <string>

namespace CrashoutsEngine
{
    // Checks:
    //  - does a .cs file exist for this class?
    //  - does the file contain [ScriptBehaviour] ?
    bool IsValidScriptBehaviourClass(const std::string& className);
}
