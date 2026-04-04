/*!
@file   debugdefaults.cpp
@author Mohamad Amir Akmal (58%), Ranvitha (42%)
@date   25/09/2025

@brief  Accessor for shared debug defaults.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "debugdefaults.h"

/*!
 @brief Accessor for shared debug defaults.
 @return Reference to a process-lifetime DebugDefaults instance.
*/
DebugDefaults& GetDebugDefaults() {
    static DebugDefaults D;
    return D;
}