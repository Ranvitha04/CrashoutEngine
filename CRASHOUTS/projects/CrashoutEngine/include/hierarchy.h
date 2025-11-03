
#pragma once
#include "entitymanager.h"

namespace CrashoutsEngine {
    void AttachChild(ECS& ecs, Entity parent, Entity child);
    void DetachChild(ECS& ecs, Entity child);
    void Reparent(ECS& ecs, Entity child, Entity newParent);

    
    void UpdateHierarchy(ECS& ecs);
}
