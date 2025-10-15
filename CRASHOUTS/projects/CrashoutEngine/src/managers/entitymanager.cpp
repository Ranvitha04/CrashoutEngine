/*!
@file        entitymanager.cpp
@author      lee.r@digipen.edu
@date        13/09/2025

This file implements 

*//*__________________________________________________________________________*/

/* _________________________________________________________________________
----------------------------------------------------------------------------- */

#include "managers/entitymanager.h"
using Entity = uint32_t;
/*!
	@brief  Destroys a specific entity.

	This function removes the entity's ID from the system and
	iterates through all component arrays to remove any associated components, ensuring a complete and clean removal.

	@param  entityId
	The ID of the entity to be destroyed.
	*/
void CrashoutsEngine::ECS::DestroyEntity(CrashoutsEngine::Entity entityId) {
	// Iterate through all component arrays to remove components
	for (auto const& pair : componentArrays) {
		pair.second->RemoveComponent(entityId);
	}
}

/*!
@brief  Destroys an entity by its name.

This function finds the entity's ID using its name and then calls the
ID-based `DestroyEntity` function to perform the removal.

@param  name
The name of the entity to be destroyed.
*/
void CrashoutsEngine::ECS::DestroyEntity(std::string name) {
	Entity entityId = FindEntityByName(name);
	// Iterate through all component arrays to remove components
	for (auto const& pair : componentArrays) {
		pair.second->RemoveComponent(entityId);
	}
}

/*!
@brief Clones an entity, copying all its components to a new entity ID.

@param srcEntityId
The ID of the entity to clone from.

@return Entity
The ID of the newly created clone entity.
*/
CrashoutsEngine::Entity CrashoutsEngine::ECS::CloneEntity(CrashoutsEngine::Entity srcEntityId) {
	// Create new entity ID
	Entity dstEntityId = nextEntityId++;

	// Iterate over all component arrays
	for (auto const& pair : componentArrays) {
		// pair.second is std::unique_ptr<IComponentArray>
		IComponentArray* componentArray = pair.second.get();

		// Call clonecomponent for this type
		componentArray->CloneComponent(srcEntityId, dstEntityId);
	}

	NameComponent* newName = GetComponent<NameComponent>(dstEntityId);
	if (newName) newName->name += "_Clone" + std::to_string(dstEntityId);

	return dstEntityId;
}

/*!
@brief Finds an entity by its name.

This function iterates through all entities that have a `NameComponent`
and returns the first entity ID that matches the provided name.

@param name
The name of the entity to search for.

@return Entity
The ID of the found entity, or 0 if no entity with that name exists.
*/
CrashoutsEngine::Entity CrashoutsEngine::ECS::FindEntityByName(std::string name) {
    // Check if NameComponent array exists
    // if (!HasComponent<NameComponent>(0)) return 0;

    auto& nameCompArray = GetComponentArray<NameComponent>();
    for (size_t i = 0; i < nameCompArray.dense.size(); ++i) {
        if (nameCompArray.dense[i].name == name) {
            return nameCompArray.indexToEntity[i];
        }
    }

    // Return 0 if not found
    return 0;
}