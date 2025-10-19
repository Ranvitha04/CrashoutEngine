#pragma once
#pragma warning(disable: 4251)
#include "inputmanager.h"   
#include "key.h"            
#include "physicsmanager.h"        
#include "ImportExport.h"
namespace Player {


	DLL_API void UpdateVelocity(Component::RigidBodyComponent& rb, float moveSpeed);

} 