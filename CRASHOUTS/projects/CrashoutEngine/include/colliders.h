/*!
@file       colliders.h
@author     miratristan.d@digipen.edu
@date       26/10/2025
@brief      Type declarations for the different types of colliders

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.


//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include <memory>
#include <map>
#include <array>
#include <string>
#include <optional>
#include <initializer_list>
#include "entitymanager.h"

namespace Collision {

	/*!
	@enum	ColliderType
	@brief	Different types of colliders
	*/
	enum class ColliderType {
		CIRCLE,
		AABB,
		OBB,
		LINE,
		TRIANGLE,
		POLYGON,
		NONE
	};

	/*enum class ColliderType {
		CIRCLE,
		AABB,
		OBB,
		TRIANGLE,
		POLYGON,
		NONE
	};*/

	/*!
	@struct	CollisionManifold
	@brief	Contains information regarding Collision events between Entities
	*/
	struct CollisionManifold {
		CEVec3 normal;
		std::array<CEVec3, 2> contactPoints; /*Max contact point of 2 for ColliderType::AABB, 1 for Circle*/
		f32 depth;
		bool collisionOccured;
		int numOfContacts;
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
		virtual void UpdateCollider([[maybe_unused]] CEVec3 const& newPos) = 0;
		virtual void UpdateRotation([[maybe_unused]] f32 newRotation) {};
		virtual f32 CalculateInertia([[maybe_unused]] f32 mass) { return 0.f; };
		CEVec3 offset{};
		CEVec3 position{};
		ColliderType ColliderType; /*HELPER FOR UI DROPDOWN*/
		bool isContinuous = false;
		bool isTrigger = false;
		bool enabled = true;

		// --- AABB CACHING ---
		mutable CEVec3 cachedMin;
		mutable CEVec3 cachedMax;
		mutable bool isAABBCacheDirty = true;
	};

	/*!
	@class	Circle
	@brief	Basic Circle collider with radius and position
	*/
	class Circle : public Collider {
	public:
		f32 radius;
		Circle(f32 radius, CEVec3 const& position, CEVec3 const& offset = CEVec3{});
		~Circle() = default;
		virtual void UpdateCollider(CEVec3 const& newPos);

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<Circle>(*this);
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

		virtual void UpdateCollider(CEVec3 const& newPos);
		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<AABBRect>(*this);
		}
	};

	/*!
	@class OBBRect
	@brief ColliderType::OBB rectangle with a collection of Vertices
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
		virtual void UpdateRotation(f32 newRotation);
		~OBBRect() = default;
		virtual void UpdateCollider(CEVec3 const&);

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<OBBRect>(*this);
		}
	private:

	};

	/*!
	@class Polygon
	@brief Polygon shape with a collection of Vertices
	*/
	class Polygon : public Collider {
		/*NORMAL AND VERTEX FOR THE PAIR*/
	public:
		constexpr static int MAX_VERTEX{ 10 };

		using SupportPoint = std::optional<std::pair<CEVec3, f32>>; //Point and Vertex;

		using Vertices = std::array<CEVec3, MAX_VERTEX>;
		using Normal = std::array<CEVec3, MAX_VERTEX>;

		float rotation{};
		int vertexCount{};

		CEVec3 extents{ 1.0f, 1.0f, 0.0f };

		Vertices vertices;
		Vertices rotatedVertices;
		Normal normalEdges;

		Polygon();
		Polygon(std::initializer_list<CEVec3> vertices, f32 rotation = 0.f, CEVec3 const& position = {}, CEVec3 const& offset = {});
		virtual void UpdateRotation(f32 newRotation);
		~Polygon() = default;
		virtual void UpdateCollider(CEVec3 const&);
		void UpdatePosition(CEVec3 const& newPos);
		void CalcNormals();
		//Re-scales vertices based on a new size
		void Rescale(const CEVec3& newExtents);
		CEVec3 CalcNormal(CEVec3 const& edge);

		virtual f32 CalculateInertia(f32 mass) override;

		void ForceUpdateRotation(f32 newRotation);

		/*ColliderType::OBB HELPER FUNCTIONS*/
		static Polygon CreateOBB(CEVec3 const& min, CEVec3 const& max, f32 rotation, CEVec3 offset);
		static void UpdateOBB(CEVec3 const& min, CEVec3 const& max, Polygon& polygon);
		static CEVec3 CalculateOBBExtents(Polygon const& polygon);

		/*ColliderType::AABB HELPER FUNCTIONS*/
		static Polygon CreateAABB(CEVec3 const& min, CEVec3 const& max, CEVec3 offset);

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<Polygon>(*this);
		}

	};

	/*!
	@class LineSegment
	@brief	Struct for the Line Segment collider
	*/
	class LineSegment : public Collider {
	public:
		// GETTERS
		const CEVec3& GetWorldP1() const { return worldP1; }
		const CEVec3& GetWorldP2() const { return worldP2; }
		const CEVec3& GetNormal() const { return normal; }

		// SETTERS
		void SetWorldP1(CEVec3& newP1) { worldP1 = newP1; }
		void SetWorldP2(CEVec3& newP2) { worldP2 = newP2; }
		void SetNormal(CEVec3& newNormal) { normal = newNormal; }

		/*
		* @brief Constructor for LineSegment struct
		* @param start - Start vertex
		* @param end - End vertex
		*/
		LineSegment(const CEVec3& start, const CEVec3& end);

		/*
		* @brief Override function personalized for Line Segement
		* @param newPos - New Position data
		*/
		void UpdateCollider(CEVec3 const& newPos) override;

		/*
		* @brief Override function personalized for Line Segement
		* @param newRot - New rotation data in degrees
		*/
		void UpdateRotation(f32 newRotation) override;

		virtual std::unique_ptr<Collider> clone() const override {
			return std::make_unique<LineSegment>(*this);
		}

		CEVec3 p1;		// Local Start
		CEVec3 p2;		// Local End

	private:
		CEVec3 worldP1;		// World Start
		CEVec3 worldP2;		// World End
		CEVec3 normal;	// Normal perpendicular to the line
	};

	class ColliderFactory
	{

	public:
		using ArgMap = std::map<std::string, std::string>;

		ColliderFactory(ArgMap const& args) : args{ args } {}

		ColliderFactory() = default;

		std::unique_ptr<Collider> CreateCollider(ColliderType type) {
			if (type == ColliderType::CIRCLE) {

				return std::make_unique<Circle>(Circle{
					std::stof(defaultedArg("radius","0.f")),
					CEVec3{
						std::stof(defaultedArg("centerX", "0.f")),
						std::stof(defaultedArg("centerY","0.f")),
						std::stof(defaultedArg("centerZ", "0.f"))
					}
					});

			} if (type == ColliderType::AABB) {

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

			} if (type == ColliderType::OBB) {

				// Use Polygon for OBB implementation
				// 1. Calculate half-width and half-height
				float w = std::stof(defaultedArg("maxX", "0.5")) - std::stof(defaultedArg("minX", "-0.5"));
				float h = std::stof(defaultedArg("maxY", "0.5")) - std::stof(defaultedArg("minY", "-0.5"));
				CEVec3 min = { -w * 0.5f, -h * 0.5f, 0.f };
				CEVec3 max = { w * 0.5f, h * 0.5f, 0.f };

				// 2. Create using Polygon's OBB helper
				auto poly = std::make_unique<Polygon>(Polygon::CreateOBB(min, max, std::stof(defaultedArg("rotation", "0.f")), CEVec3{}));
				poly->ColliderType = ColliderType::OBB; // Preserve the OBB Type for UI/Logic
				return poly;

			} if (type == ColliderType::LINE) {

				return std::make_unique<LineSegment>(
					CEVec3{ -0.5f, 0.f, 0.f }, // Default Start (p1)
					CEVec3{ 0.5f, 0.f, 0.f }  // Default End (p2)
				);

			} if (type == ColliderType::TRIANGLE) {
				// Default Triangle (Empty or basic quad)
				return std::make_unique<Polygon>(
					std::initializer_list<CEVec3>{
						{-0.5f, -0.5f, 0.f}, { 0.5f, -0.5f, 0.f }, { 0.5f, 0.5f, 0.f }
				}
				);
			}
			/*
			if (type == ColliderType::POLYGON || type == ColliderType::TRIANGLE) {
				// Default Polygon/Triangle (Empty or basic quad)
				return std::make_unique<Polygon>(
					std::initializer_list<CEVec3>{
						{-0.5f, -0.5f, 0.f}, { 0.5f, -0.5f, 0.f }, { 0.5f, 0.5f, 0.f }
				}
				);
			}*/

			return std::make_unique<Circle>(Circle{ 0.f, CEVec3{} });
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