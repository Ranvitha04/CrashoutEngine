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
#include <unordered_map>
#include <functional>

namespace CrashoutsEngine {

	struct ContactInfo {
		Collision::CollisionManifold info;
		Entity collidedEntity;
	};

	class CollisionEventManager : public Messaging::Observer {
	public:
		enum Type { Trigger, Collidable };

		using CallbackFnCollide = std::function<void(Entity)>;
		using CallbackTriggerFn = std::function<void(Entity)>;


		class Callable {
		protected:
			Callable(Type type) : type{ type } {}
		public:
			Type type;
			virtual ~Callable() = default;
		};

		class ColliderFnCollection : public Callable {
		public:
			ColliderFnCollection(CallbackFnCollide enter, CallbackFnCollide stay, CallbackFnCollide exit) : Callable{ Collidable }, enter{ enter }, stay{ stay }, exit{ exit } {}

			CallbackFnCollide enter;
			CallbackFnCollide stay;
			CallbackFnCollide exit;
		};

		class TriggerFnCollection : public Callable {
		public:
			TriggerFnCollection(CallbackTriggerFn enter, CallbackTriggerFn stay, CallbackTriggerFn exit) : Callable{ Trigger }, enter{ enter }, stay{ stay }, exit{ exit } {}

			CallbackTriggerFn enter;
			CallbackTriggerFn stay;
			CallbackTriggerFn exit;
		};


		using CollisionEventMap = std::unordered_map<CrashoutsEngine::Entity, std::unique_ptr<Callable>>;

	public:

		CollisionEventManager();

		static CollisionEventManager& GetInstance() {
			static CollisionEventManager instance; // Meyer's Singleton
			return instance;
		}

		void Clear() { collisionEvents.clear(); }
		void initEvent(CrashoutsEngine::Entity entity, Type type);

		void registerCollideEnter(CrashoutsEngine::Entity entity, CallbackFnCollide fn);
		void registerCollideStay(CrashoutsEngine::Entity entity, CallbackFnCollide fn);
		void registerCollideExit(CrashoutsEngine::Entity entity, CallbackFnCollide fn);

		void registerTriggerEnter(CrashoutsEngine::Entity entity, CallbackTriggerFn fn);
		void registerTriggerStay(CrashoutsEngine::Entity entity, CallbackTriggerFn fn);
		void registerTriggerExit(CrashoutsEngine::Entity entity, CallbackTriggerFn fn);

		inline CollisionEventMap& getCollisionEvents() {
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
		CollisionEventMap collisionEvents;
		std::unordered_map<Entity, ContactInfo> lastContacts;

	};
}