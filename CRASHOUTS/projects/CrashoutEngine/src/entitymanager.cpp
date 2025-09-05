/*!
@file       entitymanager.cpp
@author     lee.r@digipen.edu
@date       05/09/2025

This file implements the ECS (Entity-Component-System) manager, which handles
the creation, destruction, and component management of all entities.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "../include/entitymanager.h"
#include "utility"

/*
----------------------------------------------------------------------------- */
namespace framework {
	/*!
	@brief  Destroys a specific entity.

	This function removes the entity's ID from the system and
	iterates through all component arrays to remove any associated components, ensuring a complete and clean removal.

	@param  entityId
	The ID of the entity to be destroyed.
	*/
	void ECS::DestroyEntity(Entity entityId) {
		// Iterate through all component arrays to remove components
		for (auto const& pair : componentArrays) {
			pair.second->removeComponent(entityId);
		}
	}

	/*!
	@brief  Adds a component to a specified entity.

	This templated function adds a new component of type `T` to the entity. It constructs the component in-place,
	updating the sparse-set data structures to map the entity ID to the component's index in the dense array.

	@tparam T
	The type of the component to be added.

	@tparam Args
	The types of the arguments for the component's constructor.

	@param  entityId
	The ID of the entity to which the component will be added.

	@param  args
	The arguments to pass to the component's constructor.

	@return T&
	A reference to the newly added component.
	*/
	template<typename T, typename... Args>
	T& ECS::AddComponent(Entity entityId, Args&&... args) {
		auto& componentArray = GetComponentArray<T>();

		// Construct component in-place
		componentArray.dense.emplace_back(std::forward<Args>(args)...);

		size_t newIndex = componentArray.dense.size() - 1;
		// Save the entity's index in the component sparse set
		componentArray.entityToIndex[entityId] = newIndex;
		// add the new component to the back of values
		componentArray.indexToEntity.push_back(entityId);

		return componentArray.dense.back();
	}

	/*!
	@brief	Checks if an entity has a specific component.

	This templated function verifies the existence of a component of type `T` on an entity by checking the sparse - set mapping.

	@tparam	T
	The type of the component to check for.

	@param  entityId
	The ID of the entity to query.

	@return bool
	`true` if the entity has the component, `false` otherwise.
	*/
	template<typename T>
	bool ECS::HasComponent(Entity entityId) {
		auto& componentArray = GetComponentArray<T>();
		return componentArray.entityToIndex.count(entityId) > 0;
	}

	/*!
	@brief  Retrieves a component from a specific entity.

	This templated function returns a pointer to a component of type `T` for the given entity.
	It returns `nullptr` if the component does not exist.

	@tparam T
	The type of the component to retrieve.

	@param  entityId
	The ID of the entity to query.

	@return T*
	A pointer to the component, or `nullptr` if not found.
	*/
	template<typename T>
	T* ECS::GetComponent(Entity entityId) {
		if (!HasComponent<T>(entityId)) {
			return nullptr;
		}
		auto& componentArray = GetComponentArray<T>();
		size_t index = componentArray.entityToIndex.at(entityId);
		return &componentArray.dense.at(index);
	}

	/*!
	@brief  Removes a component from a specific entity.

	This templated function performs an O(1) removal of a component by using the sparse-set method. It swaps the component to be removed with the last component in the dense array and then removes the last element.

	@tparam T
	The type of the component to remove.

	@param  entityId
	The ID of the entity whose component will be removed.
	*/
	template<typename T>
	void ECS::ComponentArray<T>::RemoveComponent(Entity entityId) {
		// quit early
		if (entityToIndex.find(entityId) == entityToIndex.end()) return;

		// reference end and current entity's value
		size_t indexToRemove = entityToIndex.at(entityId);
		Entity lastEntity = indexToEntity.back();

		if (indexToRemove != dense.size() - 1) {
			// Swap component data
			dense[indexToRemove] = std::move(dense.back());

			// Update the index mappings for the swapped entity
			entityToIndex.at(lastEntity) = indexToRemove;
			indexToEntity[indexToRemove] = lastEntity;
		}

		// Remove the entries from the sparse-set structures
		entityToIndex.erase(entityId);
		dense.pop_back();
		indexToEntity.pop_back();
	}

	/*!
	@brief  Retrieves the component array for a specific component type.

	This templated helper function finds or creates the `ComponentArray` for type `T`. It uses `std::type_index` to handle a type - safe lookup in the internal map.

	@tparam T
	The type of the component array to retrieve.

	@return ComponentArray<T>&
	A reference to the component array for type `T`.
	*/
	template<typename T>
	ECS::ComponentArray<T>& ECS::GetComponentArray() {
		std::type_index typeIndex = typeid(T);
		// quit early
		if (componentArrays.find(typeIndex) == componentArrays.end()) {
			componentArrays[typeIndex] = std::make_unique<ComponentArray<T>>();
		}
		return static_cast<ComponentArray<T>&>(*componentArrays.at(typeIndex));
	}
}