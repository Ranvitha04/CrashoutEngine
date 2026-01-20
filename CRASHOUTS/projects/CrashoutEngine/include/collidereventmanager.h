#pragma once

/*!
@file       collidereventmanager.h
@author     miratristan.d@digipen.edu
@date       29/11/2025
@brief      colliderevent manager declaration

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#include "entitymanager.h"
#include "collisionmanager.h"
#include "collisionutils.h"
#include "observer.h"
#include "messages.h"
#include <unordered_map>
#include <functional>

namespace CrashoutsEngine {

	struct ContactInfo {
		Collision::CollisionManifold info;
		Entity collidedEntity;
	};

	class CollisionEventManager : public Messaging::Observer {
	public:
		using CallbackFn = std::function<void(Entity)>;

		/*
		@brief Allows one entity to hold collider and trigger callbacks
		*/
		struct EntityEventCollection {
			// Physics Collision Callbacks
			CallbackFn onCollisionEnter = nullptr;
			CallbackFn onCollisionStay = nullptr;
			CallbackFn onCollisionExit = nullptr;

			// Trigger Overlap Callbacks
			CallbackFn onTriggerEnter = nullptr;
			CallbackFn onTriggerStay = nullptr;
			CallbackFn onTriggerExit = nullptr;
		};

		using CollisionEventMap = std::unordered_map<CrashoutsEngine::Entity, EntityEventCollection>;

		CollisionEventManager();

		static CollisionEventManager& GetInstance() {
			static CollisionEventManager instance; // Meyer's Singleton
			return instance;
		}

		void Clear() { 
			CollisionEventMap().swap(collisionEvents); 
			std::unordered_map<Entity, ContactInfo>().swap(lastContacts);
		}

		using PreSolveFn = std::function<bool(Entity)>;
		std::unordered_map<Entity, PreSolveFn> preSolveCallbacks;

		void RegisterPreSolve(Entity e, PreSolveFn fn);
		bool CheckPreSolve(Entity me, Entity other);

		void RegisterCollideEnter(CrashoutsEngine::Entity entity, CallbackFn fn);
		void RegisterCollideStay(CrashoutsEngine::Entity entity, CallbackFn fn);
		void RegisterCollideExit(CrashoutsEngine::Entity entity, CallbackFn fn);
			 
		void RegisterTriggerEnter(CrashoutsEngine::Entity entity, CallbackFn fn);
		void RegisterTriggerStay(CrashoutsEngine::Entity entity, CallbackFn fn);
		void RegisterTriggerExit(CrashoutsEngine::Entity entity, CallbackFn fn);

		inline CollisionEventMap& GetCollisionEvents() {
			return collisionEvents;
		}

		bool TryGetLastContact(Entity entity, ContactInfo& outInfo) const {
			auto it = lastContacts.find(entity);
			if (it == lastContacts.end())
				return false;
			outInfo = it->second;
			return true;
		}

		void SetLastContact(Entity entity, ContactInfo const& info) {
			lastContacts[entity] = info;
		}

		~CollisionEventManager() = default;

	private:
		// Helper to get and create the event struct safely
		EntityEventCollection& GetEvents(Entity entity);

		CollisionEventMap collisionEvents;
		std::unordered_map<Entity, ContactInfo> lastContacts;

	};
}