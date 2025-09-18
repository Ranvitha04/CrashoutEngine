/*!
@file   entitymanager.h
@author lee.r@digipen.edu
@date   05/09/2025

@brief	This file contains the declaration and implementation of the 
		**ECS (Entity-Component-System)** framework. This file implements 
		the core ECS classes: `ECS` (the main manager), `IComponentArray` 
		(an abstract interface for component containers), and 
		`ComponentArray<T>` (a templated class for storing components of a 
		specific type). The system uses a sparse-set data structure for 
		efficient O(1) component addition, retrieval, and removal, optimizing 
		memory layout for performance. It also includes utility functions for
		managing entities by both their ID and name.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <utility>

// Include components here:
#include "components/namecomponent.h"
#include "components/transformcomponent.h"
#include "shapes.h"

/*                                                                   
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {
	using namespace Component;

	using Entity = uint32_t;

	class IComponentArray;

	/*!
	@class ECS
	@brief The core Entity-Component-System (ECS) manager.

	This class serves as the central hub for the ECS framework. It is responsible for creating,
	destroying, and managing entities and their associated components. It maintains a collection
	of component arrays, one for each component type, enabling efficient data access and manipulation.
	*/
	class ECS {
	public:
		// Create a new entity and return ID
		Entity CreateEntity() {
			Entity newId = nextEntityId++;
			return newId;
		}

		// Find Entity by name
		Entity FindEntityByName(std::string name);

		// Add component to entity
		template<typename T, typename... Args>
		T& AddComponent(Entity entityId, Args&&... args);

		// Option to add component to entity using entity name
		template<typename T, typename... Args>
		T& AddComponent(std::string entityName, Args&&... args);

		// Remove component from entity
		template<typename T>
		void RemoveComponent(Entity entityId);

		// Option to remove component from entity using entity name
		template<typename T>
		void RemoveComponent(std::string entityName);

		// Get component to entity
		template<typename T>
		T* GetComponent(Entity entityId);

		// Option to get component of entity using entity name
		template<typename T>
		T* GetComponent(std::string entityName);

		// Destroy entity
		void DestroyEntity(Entity entityId);

		// Option to destroy entity using entity name
		void DestroyEntity(std::string entityName);

		// Check entity if has component
		template<typename T>
		bool HasComponent(Entity entityId);

		// Option to check if entity has component using entity name
		template<typename T>
		bool HasComponent(std::string entityName);

	private:
		Entity nextEntityId{ 1 };
		// Store unique references to each component array of different type
		std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> componentArrays;

		template<typename T>
		class ComponentArray;
		
		template<typename T>
		ComponentArray<T>& GetComponentArray();
	};

	/*!
	@class IComponentArray
	@brief An abstract interface for component arrays.

	This base class provides a polymorphic way to remove components from an entity
	without knowing their specific type. This is particularly useful when destroying an
	entity, as the ECS can iterate through all component arrays and call a single
	`RemoveComponent` function for each one.
	*/
	class IComponentArray {
	public:
		virtual	~IComponentArray() = default;
		virtual void RemoveComponent(Entity entityId) = 0;
	};

	/*!
	@class ComponentArray
	@brief A templated array for storing a specific type of component.

	This class is the core of the ECS's data storage. It uses a sparse-set data
	structure to store components of type `T` in a contiguous vector (`dense`).
	It maintains efficient mappings between entity IDs and component indices,
	allowing for fast component access and O(1) removals.
	*/
	template<typename T>
	class ECS::ComponentArray : public IComponentArray {
	public:
		void RemoveComponent(Entity entityId) override;

		// Public members for dense set implementation
		// Component values stored in dense
		std::vector<T> dense;
		std::unordered_map<Entity, size_t> entityToIndex;
		std::vector<Entity> indexToEntity;
	};

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
	@brief  Adds a component to an entity found by its name.

	This function finds the entity's ID by name and then calls the ID-based
	`AddComponent` to attach the component.

	@tparam T
	The type of the component to add.

	@tparam Args
	The types of the arguments for the component's constructor.

	@param  name
	The name of the entity.

	@param  args
	The arguments to pass to the component's constructor.

	@return T&
	A reference to the newly added component.
	*/
	template<typename T, typename... Args>
	T& ECS::AddComponent(std::string name, Args&&... args) {
		Entity entityId = FindEntityByName(name);

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
	@brief Removes a component from a specified entity.

	This templated function retrieves the component array for the specified type
	and calls the component array's `RemoveComponent` method to handle the removal.

	@tparam T
	The type of the component to remove.

	@param entityId
	The ID of the entity whose component will be removed.
	*/
	template<typename T>
	void ECS::RemoveComponent(Entity entityId) {
		GetComponentArray<T>().RemoveComponent(entityId);
	}

	/*!
	@brief Removes a component from an entity found by its name.

	This function finds the entity's ID by name and then calls the ID-based
	`RemoveComponent` to remove the component.

	@tparam T
	The type of the component to remove.

	@param name
	The name of the entity.
	*/
	template<typename T>
	void ECS::RemoveComponent(std::string name) {
		Entity entityId = FindEntityByName(name);
		GetComponentArray<T>().RemoveComponent(entityId);
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
	@brief Checks if an entity, found by its name, has a specific component.

	This function finds the entity's ID by name and then calls the ID-based
	`HasComponent` to check for the component.

	@tparam T
	The type of the component to check for.

	@param name
	The name of the entity.

	@return bool
	`true` if the entity has the component, `false` otherwise.
	*/
	template<typename T>
	bool ECS::HasComponent(std::string name) {
		Entity entityId = FindEntityByName(name);
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
	@brief  Retrieves a component from an entity found by its name.

	This function finds the entity's ID by name and then calls the ID-based
	`GetComponent` to retrieve the component pointer.

	@tparam T
	The type of the component to retrieve.

	@param  name
	The name of the entity.

	@return T*
	A pointer to the component, or `nullptr` if not found.
	*/
	template<typename T>
	T* ECS::GetComponent(std::string name) {
		Entity entityId = FindEntityByName(name);
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

