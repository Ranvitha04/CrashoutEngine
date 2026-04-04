/*!
@file   collidereventmanager.cpp
@author Shahrin Idzuan (55%), Tristan Mira (13%), Renfred Lee (32%)
@date   29/11/2025

@brief  collidereventmanager.cpp file contains the implementation of the CollisionEventManager class for handling collision events in the CrashoutsEngine

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "collidereventmanager.h"

namespace CrashoutsEngine {

	/*
	@brief	Creates or retrieves events from an entity
	@param	entity - entity to get event from
	@return events
	*/
	CollisionEventManager::EntityEventCollection& CollisionEventManager::GetEvents(Entity entity) {
		return collisionEvents[entity];
	}

	/*
	@brief Template function to invoke callback
	@param fn		- Function to invoke
	@param other	- Collided entity to invoke function on
	*/
	template <typename Func>
	static void TryInvoke(const Func& fn, Entity other) {
		if (fn) fn(other);
	}

	/*ENTER, STAY, EXIT EVENT HANDLERS*/
	static void HandleCollisionMessages(Messaging::Messages& msgBase, Messaging::Observer& obsBase) {

		auto& msg = static_cast<Collision::CollisionMessage&>(msgBase);
		auto& observer = static_cast<CollisionEventManager&>(obsBase);

		// 1. Get Data from Message
		Collision::ContactPair pair = msg.GetCollisionPair();
		Messaging::EventType type = msg.GetType();

		CollisionEventManager::CollisionEventMap& eventsMap = observer.GetCollisionEvents();

		// 2. Lambda to handle one side of the interaction
		auto HandleSide = [&](Entity me, Entity other) {
			CollisionEventManager::CollisionEventMap::iterator it = eventsMap.find(me);
			if (it == eventsMap.end()) return;

			// 3. Check collision type and invoke
			switch (type) {
				// Physics Collisions
				case Messaging::EventType::CollisionEnter: TryInvoke(it->second.onCollisionEnter, other); break;
				case Messaging::EventType::CollisionStay: TryInvoke(it->second.onCollisionStay, other); break;
				case Messaging::EventType::CollisionExit: TryInvoke(it->second.onCollisionExit, other); break;

				// Trigger Overlaps
				case Messaging::EventType::TriggerEnter: TryInvoke(it->second.onTriggerEnter, other); break;
				case Messaging::EventType::TriggerStay: TryInvoke(it->second.onTriggerStay, other); break;
				case Messaging::EventType::TriggerExit: TryInvoke(it->second.onTriggerExit, other); break;

				default: break;
			}
		};

		// 4. Run for both entities colliding or overlapping
		HandleSide(pair.first, pair.second);
		HandleSide(pair.second, pair.first);
	}

	/*-----------------------------------*/

	CollisionEventManager::CollisionEventManager() : Messaging::Observer{} {
		using namespace Messaging;

		HANDLER handler = HandleCollisionMessages;

		attachHandler(EventType::CollisionEnter, handler);
		attachHandler(EventType::CollisionStay, handler);
		attachHandler(EventType::CollisionExit, handler);

		attachHandler(EventType::TriggerEnter, handler);
		attachHandler(EventType::TriggerStay, handler);
		attachHandler(EventType::TriggerExit, handler);
	}

	void CollisionEventManager::RegisterPreSolve(Entity e, PreSolveFn fn) { preSolveCallbacks[e] = fn; }
	bool CollisionEventManager::CheckPreSolve(Entity me, Entity other) {
		if (preSolveCallbacks.count(me)) return preSolveCallbacks[me](other);
		return true; // Default allow
	}

	void CollisionEventManager::RegisterCollideEnter(Entity entity, CallbackFn fn) {
		GetEvents(entity).onCollisionEnter = fn;
	}
	void CollisionEventManager::RegisterCollideStay(Entity entity, CallbackFn fn) {
		GetEvents(entity).onCollisionStay = fn;
	}
	void CollisionEventManager::RegisterCollideExit(Entity entity, CallbackFn fn) {
		GetEvents(entity).onCollisionExit = fn;
	}

	void CollisionEventManager::RegisterTriggerEnter(Entity entity, CallbackFn fn) {
		GetEvents(entity).onTriggerEnter = fn;
	}
	void CollisionEventManager::RegisterTriggerStay(Entity entity, CallbackFn fn) {
		GetEvents(entity).onTriggerStay = fn;
	}
	void CollisionEventManager::RegisterTriggerExit(Entity entity, CallbackFn fn) {
		GetEvents(entity).onTriggerExit = fn;
	}

}