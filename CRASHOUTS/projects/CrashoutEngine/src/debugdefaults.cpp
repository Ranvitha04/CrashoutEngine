/*!
@file       debugdefaults.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

Defines editor tuning defaults used across Debug Menu adapters
(e.g., drag speeds, initial rigidbody values, default collider sizes).

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

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