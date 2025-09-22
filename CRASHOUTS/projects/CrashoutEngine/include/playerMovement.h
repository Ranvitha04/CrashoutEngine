#pragma once

#include "managers/inputmanager.h"   
#include "key.h"            
#include "managers/physicsmanager.h"        

namespace Player {


	void UpdateVelocity(Component::RigidBodyComponent& rb, float moveSpeed);

} 