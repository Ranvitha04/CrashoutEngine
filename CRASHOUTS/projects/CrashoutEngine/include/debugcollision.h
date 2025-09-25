#pragma once

#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"
#include "components/transformcomponent.h"
#include "components/shapescomponent.h"
#include "shapes.h"

namespace CollisionGizmos {

    // Colliding ones are tinted if you pass the collision events.
    void DrawAllColliders(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>* events = nullptr, float wireThickness = 0.02f);

    // Draw contact points + normals from collision results.
    void DrawContactsAndNormals(const std::vector<Collision::CollisionEvent>& events, float normalLen = 0.25f, float wireThickness = 0.02f);

} // namespace CollisionGizmos