#pragma once
#include <vector>
#include "entitymanager.h"

namespace CrashoutsEngine {
	namespace Component {
		struct ParentComponent { Entity parent = 0; };                
		struct ChildrenComponent { std::vector<Entity> children = {}; }; 
	}
} // namespace
