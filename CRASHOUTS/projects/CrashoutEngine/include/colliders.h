/*!
@file       colliders.h
@author     miratristan.d@digipen.edu
@date       26/10/2025
@brief      Type declarations for the different types of colliders


//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include <memory>
#include <map>
#include <array>
#include <string>
#include "entitymanager.h"

namespace Collision {

	/*!
	@enum	ColliderType
	@brief	Different types of colliders
	*/
	enum ColliderType {
		SPHERE,
		AABB,
		OBB
	};

	/*!
	@struct	CollisionManifold
	@brief	Contains information regarding Collision events between Entities
	*/
	struct CollisionManifold {
		CEVec3 normal;
		std::array<CEVec3, 2> contactPoints; /*Max contact point of 2 for AABB, 1 for Circle*/
		f32 depth;
		bool collisionOccured;
	};

	/*!
	@struct	CollisionEvent
	@brief	Contains which pairs of entity has collided along with their Collision manifold
	*/
	struct CollisionEvent {
		CollisionManifold colManifold;
		CrashoutsEngine::Entity entity1;
		CrashoutsEngine::Entity entity2;
	};

	/*!
	@class	Collider
	@brief	Base class for inheriting other colliders
	*/
	class Collider {
	public:
		ColliderType type;
		Collider(ColliderType type);
		virtual ~Collider() = default;
		virtual std::unique_ptr<Collider> clone() const = 0;
		virtual void updateCollider([[maybe_unused]] CEVec3 const& newPos) {};
	};

	/*!
	@class	Sphere
	@brief	Basic Sphere collider with radius and position
	*/
	class Sphere : public Collider {
	public:
		f32 radius;
		CEVec3 center;
		Sphere(f32 radius, CEVec3 const& center);
		~Sphere() = default;
		virtual void updateCollider(CEVec3 const& newPos);

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<Sphere>(*this);
		}
	};

	/*!
	@class	AABBRect
	@brief	Basic aabb rectangle with CEVec3 min and CEVec3 max
	*/
	class AABBRect : public Collider {
	public:
		CEVec3 min;
		CEVec3 max;
		CEVec3 extents;
		AABBRect();
		AABBRect(CEVec3 const& min, CEVec3 const& max);
		//AABBRect(CEVec3 const& newExtents, CEVec3 const &transform);

		inline bool contains(AABBRect const& aabb) const { return min.x <= aabb.min.x && max.x >= aabb.max.x && min.y <= aabb.min.y && max.y >= aabb.max.y; }

		inline bool intersect(AABBRect const& aabb) const { return (min.x <= aabb.max.x && max.x >= aabb.min.x) && (min.y <= aabb.max.y && max.y >= aabb.min.y); }

		~AABBRect() = default;

		virtual void updateCollider(CEVec3 const& newPos);
		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<AABBRect>(*this);
		}
	};

	/*!
	@class OBBRect
	@brief OBB rectangle with a collection of Vertices
	*/
	class OBBRect : public Collider {
		/*NORMAL AND VERTEX FOR THE PAIR*/
	public:
		using Vertices = std::array<CEVec3, 4>;
		using Normal = std::array<CEVec3, 4>;
	public:
		CEVec3 min;
		CEVec3 max;
		float rotation;
		CEVec3 extents;
		Vertices rotatedVertices;
		Vertices originalVertices;
		Normal normalEdges;
		OBBRect(CEVec3 const& min, CEVec3 const& max, f32 rotation = 0.f);
		void updateRotation(f32 newRotation);
		~OBBRect() = default;
		virtual void updateCollider(CEVec3 const&);

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<OBBRect>(*this);
		}
	private:

	};

	class ColliderFactory
	{

	public:
		using ArgMap = std::map<std::string, std::string>;

		ColliderFactory(ArgMap const& args) : args{ args } {}

		ColliderFactory() = default;

		std::unique_ptr<Collider> CreateCollider(ColliderType type) {
			if (type == SPHERE) {

				return std::make_unique<Sphere>(Sphere{
					std::stof(defaultedArg("radius","0.f")),
					CEVec3{
						std::stof(defaultedArg("centerX", "0.f")),
						std::stof(defaultedArg("centerY","0.f")),
						std::stof(defaultedArg("centerZ", "0.f"))
					}
					});

			} if (type == AABB) {

				return std::make_unique<AABBRect>(AABBRect{
					CEVec3{
						std::stof(defaultedArg("minX", "0.f")),
						std::stof(defaultedArg("minY", "0.f")),
						std::stof(defaultedArg("minZ", "0.f"))
					}, CEVec3{
						std::stof(defaultedArg("maxX", "0.f")),
						std::stof(defaultedArg("maxY", "0.f")),
						std::stof(defaultedArg("maxZ", "0.f"))
					}
					});

			} if (type == OBB) {

				return std::make_unique<OBBRect>(OBBRect{
					CEVec3{
						std::stof(defaultedArg("minX", "0.f")),
						std::stof(defaultedArg("minY", "0.f")),
						std::stof("0.0")
					}, CEVec3{
						std::stof(defaultedArg("maxX", "0.f")),
						std::stof(defaultedArg("maxY", "0.f")),
						std::stof("0.0")
					},
					std::stof(defaultedArg("rotation", "0.f"))
					});

			}

			return std::make_unique<Sphere>(Sphere{ 0.f, CEVec3{} });
		}

		void setArgs(ArgMap newArgs) {
			args = newArgs;
		}
	private:
		ArgMap args;

		std::string defaultedArg(std::string val, std::string defaultVal) {
			if (args.find(val) == args.end())
				return defaultVal;
			else
				return args[val];
		}
	};

	AABBRect GetAABB(Collider const& collider, CEVec3 const& transform);
}