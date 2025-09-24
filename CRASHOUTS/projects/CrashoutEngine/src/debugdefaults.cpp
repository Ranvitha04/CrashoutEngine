#include "debugdefaults.h"


DebugDefaults& GetDebugDefaults() {
    static DebugDefaults D;
    return D;
}