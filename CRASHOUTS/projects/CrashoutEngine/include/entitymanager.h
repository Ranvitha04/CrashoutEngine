/*!
@file       entitymanager.h
@author     lee.r@digipen.edu
@date       05/09/2025

This file implements
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <memory>

/*                                                                   
----------------------------------------------------------------------------- */
namespace framework {

	using Entity = uint32_t;

	class IComponentArray;

	class ECS {
	public:
		// Create a new entity and return ID
		Entity CreateEntity() {
			Entity newId = nextEntityId++;
			return newId;
		}

		// Add component to entity
		template<typename T, typename... Args>
		T& AddComponent(Entity entityId, Args&&... args);

		// Get component to entity
		template<typename T>
		T* GetComponent(Entity entityId);

		// Destroy entity
		void DestroyEntity(Entity entityId);

		// Check if has component
		template<typename T>
		bool HasComponent(Entity entityId);

	private:
		Entity nextEntityId{ 0 };
		// Store unique references to each component array of different type
		std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> componentArrays;

		template<typename T>
		class ComponentArray;
		
		template<typename T>
		ComponentArray<T>& GetComponentArray();
	};

	class IComponentArray {
	public:
		virtual	~IComponentArray() = default;
		virtual void removeComponent(Entity entityId) = 0;
	};

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

}

