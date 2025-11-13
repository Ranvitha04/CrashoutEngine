#include "collidereventmanager.h"
#include "messages.h"

namespace {
	//void executeEvent(CrashoutsEngine::CollisionEventManager::CollisionEventMap::iterator &eventIter, Collision::CollisionMessage& collEvent) {
	//	using namespace CrashoutsEngine;
	//	switch (eventIter->second->type) {
	//			case CollisionEventManager::Collidable: {
	//				CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*eventIter->second) };
	//				//event.stay();
	//			}
	//			break;

	//			case CollisionEventManager::Trigger:
	//				CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*eventIter->second) };
	//				//event.stay();
	//			break;
	//	}
	//}
}

namespace CrashoutsEngine {

	/*ENTER, STAY, EXIT EVENT HANDLERS*/
	static void handleExitEvents(Collision::CollisionMessage& collEvent, CollisionEventManager& observer) {
		Collision::ContactPair pair{ collEvent.getCollisionPair() };

		CollisionEventManager::CollisionEventMap& currentEvents{ observer.getCollisionEvents() };

		CollisionEventManager::CollisionEventMap::iterator firstIter{ currentEvents.find(pair.first) };

		CollisionEventManager::CollisionEventMap::iterator secondIter{ currentEvents.find(pair.second) };

		if (firstIter != currentEvents.end()) {
			switch (firstIter->second->type) {
				case CollisionEventManager::Collidable: {
					CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*firstIter->second) };
					event.exit(pair.second);
					}
					break;

				case CollisionEventManager::Trigger:
					CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*firstIter->second) };
					event.exit(pair.second);
					break;
			}
		}

		if (secondIter != currentEvents.end()) {
			switch (secondIter->second->type) {
				case CollisionEventManager::Collidable: {
					CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*secondIter->second) };
					event.exit(pair.first);
				}
					break;

				case CollisionEventManager::Trigger:
					CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*secondIter->second) };
					event.exit(pair.first);
					break;
			}
		}

	}


	static void handleStayEvents(Collision::CollisionMessage& collEvent, CollisionEventManager& observer) {
		Collision::ContactPair pair{ collEvent.getCollisionPair() };

		CollisionEventManager::CollisionEventMap& currentEvents{ observer.getCollisionEvents() };

		CollisionEventManager::CollisionEventMap::iterator firstIter{ currentEvents.find(pair.first) };

		CollisionEventManager::CollisionEventMap::iterator secondIter{ currentEvents.find(pair.second) };

		std::cout << "HEY\n";

		if (firstIter != currentEvents.end()) {
			switch (firstIter->second->type) {
			case CollisionEventManager::Collidable: {
				CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*firstIter->second) };
				event.stay(pair.second);
			}
				break;

			case CollisionEventManager::Trigger:
				CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*firstIter->second) };
				event.stay(pair.second);
				break;
			}
		}

		if (secondIter != currentEvents.end()) {
			switch (secondIter->second->type) {
			case CollisionEventManager::Collidable: {
				CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*secondIter->second) };
				event.stay(pair.first);
			}
				break;

			case CollisionEventManager::Trigger:
				CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*secondIter->second) };
				event.stay(pair.first);
				break;
			}
		}
	}

	static void handleEnterEvents(Collision::CollisionMessage& collEvent, CollisionEventManager& observer) {
		Collision::ContactPair pair{ collEvent.getCollisionPair() };

		CollisionEventManager::CollisionEventMap& currentEvents{ observer.getCollisionEvents() };

		CollisionEventManager::CollisionEventMap::iterator firstIter{ currentEvents.find(pair.first) };

		CollisionEventManager::CollisionEventMap::iterator secondIter{ currentEvents.find(pair.second) };

		if (firstIter != currentEvents.end()) {
			switch (firstIter->second->type) {
			case CollisionEventManager::Collidable: {
				CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*firstIter->second) };
				event.enter(pair.second);
			}
				break;

			case CollisionEventManager::Trigger:
				CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*firstIter->second) };
				event.enter(pair.second);
				break;
			}
		}

		if (secondIter != currentEvents.end()) {
			switch (secondIter->second->type) {
			case CollisionEventManager::Collidable: {
				CollisionEventManager::ColliderFnCollection& event{ dynamic_cast<CollisionEventManager::ColliderFnCollection&>(*secondIter->second) };
				event.enter(pair.first);
			}
				break;

			case CollisionEventManager::Trigger:
				CollisionEventManager::TriggerFnCollection& event{ dynamic_cast<CollisionEventManager::TriggerFnCollection&>(*secondIter->second) };
				event.enter(pair.first);
				break;
			}
		}
	}
	/*-----------------------------------*/

	CollisionEventManager::CollisionEventManager() : Messaging::Observer{ "CollisionEventHandler" } {
		attachHandler(Messaging::getExitMsgID(), (Messaging::HANDLER)handleExitEvents);
		attachHandler(Messaging::getStayMsgID(), (Messaging::HANDLER)handleStayEvents);
		attachHandler(Messaging::getEnterMsgID(), (Messaging::HANDLER)handleEnterEvents);
	}

	void CollisionEventManager::initEvent(CrashoutsEngine::Entity entity, Type type) {

		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };
		if (iter == collisionEvents.end()) {
			if (type == Type::Collidable) {
				collisionEvents.insert(std::pair<CrashoutsEngine::Entity, std::unique_ptr<Callable>>{
					entity,

						std::make_unique<ColliderFnCollection>(ColliderFnCollection{
							nullptr,
							nullptr,
							nullptr
							})

				});
			}
			else {
				collisionEvents.insert(std::pair<CrashoutsEngine::Entity, std::unique_ptr<Callable>>{
					entity,

						std::make_unique<TriggerFnCollection>(TriggerFnCollection{
							nullptr,
							nullptr,
							nullptr
							})

				});
			}

		}

	}

	void CollisionEventManager::registerCollideEnter(CrashoutsEngine::Entity entity, CallbackFnCollide fn) {
		initEvent(entity, Collidable);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };
		try {
			ColliderFnCollection& event{ dynamic_cast<ColliderFnCollection&>(*iter->second) };
			event.enter = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}

	}

	void CollisionEventManager::registerCollideStay(CrashoutsEngine::Entity entity, CallbackFnCollide fn) {
		initEvent(entity, Collidable);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };

		try {
			ColliderFnCollection& event{ dynamic_cast<ColliderFnCollection&>(*iter->second) };
			event.stay = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}


	}

	void CollisionEventManager::registerCollideExit(CrashoutsEngine::Entity entity, CallbackFnCollide fn) {
		initEvent(entity, Collidable);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };

		try {
			ColliderFnCollection& event{ dynamic_cast<ColliderFnCollection&>(*iter->second) };
			event.exit = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}

	}

	void CollisionEventManager::registerTriggerEnter(CrashoutsEngine::Entity entity, CallbackTriggerFn fn) {
		initEvent(entity, Trigger);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };
		try {
			TriggerFnCollection& event{ dynamic_cast<TriggerFnCollection&>(*iter->second) };
			event.enter = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}
	}

	void CollisionEventManager::registerTriggerStay(CrashoutsEngine::Entity entity, CallbackTriggerFn fn) {
		initEvent(entity, Trigger);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };

		try {
			TriggerFnCollection& event{ dynamic_cast<TriggerFnCollection&>(*iter->second) };
			event.stay = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}

	}

	void CollisionEventManager::registerTriggerExit(CrashoutsEngine::Entity entity, CallbackTriggerFn fn) {
		initEvent(entity, Trigger);
		CollisionEventMap::iterator iter{ collisionEvents.find(entity) };

		try {
			TriggerFnCollection& event{ dynamic_cast<TriggerFnCollection&>(*iter->second) };
			event.exit = fn;
		}
		catch ([[maybe_unused]] std::bad_cast const& err) {
			return;
		}

	}


}