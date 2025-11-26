/* !
@file       scriptvalidation.h
@author     mohamadamirakmal.b@digipen.edu
@date       26/11/2025

Declares the function to validate if a given class name corresponds to a valid script behaviour class.
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