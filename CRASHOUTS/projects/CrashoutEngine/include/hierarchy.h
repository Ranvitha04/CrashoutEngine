/*!
@file        hierarchy.h
@author      b.mohamadafiq@digipen.edu
@date        7/11/2025
@brief

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include "entitymanager.h"

namespace CrashoutsEngine {
    void AttachChild(ECS& ecs, Entity parent, Entity child);
    void DetachChild(ECS& ecs, Entity child);
    void Reparent(ECS& ecs, Entity child, Entity newParent);

    bool IsAlreadyChildOf(ECS& ecs, Entity parent, Entity child);

    void UpdateHierarchy(ECS& ecs);
    void UpdateActiveHierarchy(ECS& ecs);
    void RebuildHierarchyLinks(ECS& ecs);

    Entity GetRoot(ECS& ecs, Entity e);
    void GatherDescendants(ECS& ecs, Entity e, std::vector<Entity>& out);
}

