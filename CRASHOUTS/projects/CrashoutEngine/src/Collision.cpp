/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#include "collisionmanager.h"
#include "rigidbodycomponent.h"
#include "CEMath.h"
#include "CEMtx33.h"
#include <cmath>
#include <algorithm>

namespace Collision {
	Collider::Collider(ColliderType type) : type{ type } { /*EMPTY BY DESIGN*/ }

	Sphere::Sphere(f32 radius, CEVec3 const& center) : Collider{ SPHERE }, radius{ radius }, center{ center } { /*EMPTY BY DESIGN*/ }

	void Sphere::updateCollider(CEVec3 const& newPos) {
		center = newPos;
	}

	AABBRect::AABBRect(CEVec3 const& min, CEVec3 const& max) : Collider{ AABB }, min{ min }, max{ max } {
		extents = (max - min) * 0.5f;
	}

	void AABBRect::updateCollider(CEVec3 const& newPos) {
		max = newPos + extents;
		min = newPos - extents;
	}
	OBBRect::OBBRect(CEVec3 const& min, CEVec3 const& max, f32 newRotation) : Collider{ OBB }, min{ min }, max{ max }, rotation{ 0.f } {
		extents = (max - min) * 0.5f;

		originalVertices[0] = min;

		originalVertices[1] = min;
		originalVertices[1].y += extents.y * 2.f;

		originalVertices[2] = max;

		originalVertices[3] = min;
		originalVertices[3].x += extents.x * 2.f;

		/*If angle of rotation is 0.f*/
		if (!rotation) {
			rotatedVertices = originalVertices;
			/*Normals will just be straight forward, pointing left, up, right, down for unrotated obb*/
			normalEdges[0] = CEVec3{ -1.f,0.f,originalVertices[0].z };
			normalEdges[1] = CEVec3{ 0.f,1.f,originalVertices[0].z };
			normalEdges[2] = CEVec3{ 1.f,0.f,originalVertices[0].z };
			normalEdges[3] = CEVec3{ 0.f,-1.f,originalVertices[0].z };

		}

		updateRotation(newRotation);

	}

	void OBBRect::updateRotation(f32 newRotation) {

		/*Check if rotation didnt change from last time to avoid expensive calculation*/
		if (rotation == newRotation) return;

		rotation = newRotation;
		CEMtx33 rotationMtx{};
		rotationMtx = rotationMtx.RotationDegrees(rotation);

		CEVec3 center{ min + extents };

		Vertices origin{
			min - center,
			CEVec3{min.x, min.y + extents.y * 2.f, min.z} - center,
			max - center,
			CEVec3{min.x + extents.x * 2.f, min.y, min.z} - center
		};

		rotatedVertices = origin;

		for (int i{}; i < 4; ++i) {

			rotatedVertices[i] = { rotationMtx * rotatedVertices[i] };
			rotatedVertices[i] += center;
		}

		for (int i{}; i < 4; ++i) {
			int nextVertexIdx{ (i + 1) % 4 };
			CEVec3 edge{ rotatedVertices[nextVertexIdx] - rotatedVertices[i] };
			CEVec3 rotatedEdge{ -edge.y, edge.x, edge.z };
			normalEdges[i] = rotatedEdge;
			normalEdges[i] = CEMath::Normalize(normalEdges[i]);

		}
	}

	void OBBRect::updateCollider(CEVec3 const& newPos) {
		CEVec3 currPos{ min + extents };
		CEVec3 vecToNewPos{ newPos - currPos };

		vecToNewPos.z = 0.f;

		max = newPos + extents;
		min = newPos - extents;

		rotatedVertices[0] += vecToNewPos;
		rotatedVertices[1] += vecToNewPos;
		rotatedVertices[2] += vecToNewPos;
		rotatedVertices[3] += vecToNewPos;
	}

	std::pair<f32, f32> projOBBOnAxis(std::array<CEVec3, 4> const& vertices, CEVec3 const& axis) {
		f32 min{ CEMath::Dot(vertices[0], axis) };
		f32 max{ min };

		for (CEVec3 const& vertex : vertices) {
			f32 dot{ CEMath::Dot(vertex, axis) };

			if (dot < min) {
				min = dot;
			} if (dot > max) {
				max = dot;
			}
		}
		return { min, max };
	}

	CEVec3 const* findClosestVertex(std::array<CEVec3, 4> const& vertices, CEVec3 const& point) {
		CEVec3 const* closestPt{ &vertices[0] };
		f32 minDist{ std::numeric_limits<f32>::min() };

		for (CEVec3 const& vertex : vertices) {
			CEVec3 vertexToPoint{ point - vertex };

			f32 dist{ CEMath::LengthSq(vertexToPoint) };

			if (dist < minDist) {
				closestPt = &vertex;
				minDist = dist;
			}
		}

		return closestPt;
	}

	std::pair<f32, f32> projectCircleOntoAxis(Sphere const& sphere2Temp, CEVec3 const& center, CEVec3 const& axis) {

		CEVec3 outerPoint1{ center + axis * sphere2Temp.radius };
		CEVec3 outerPoint2{ center - axis * sphere2Temp.radius };

		f32 min{ CEMath::Dot(outerPoint1, axis) };
		f32 max{ CEMath::Dot(outerPoint2, axis) };

		if (min > max) std::swap(min, max);

		return std::pair<f32, f32>{min, max};

	}
}

namespace Collision {

	/*--------------------------------------COLLISION MESSAGE DEFINITION---------------------------------------*/

	CollisionMessage::CollisionMessage(const std::string& id, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2)
		: Messages::Messages{ id }, colManifold{ colManifold }, entity1{ entity1 }, entity2{ entity2 }
	{ /*EMPTY BY DESIGN*/ }

	std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> CollisionMessage::getCollisionPair() {
		return std::pair{ entity1, entity2 };
	}

	CollisionManifold CollisionMessage::getCollisionManifold() {
		return colManifold;
	}
	/*----------------------------------------------------------------------------------------------------------*/


	/*----------------COLLISIONHANDLER CLASS DEFINITIONS--------------------------------------------------------*/

	CollisionHandler::CollisionHandler() : Messaging::Observable{ "Collision" }, collideFuncts{
		std::array<Collision::FunctionDispatchType, TYPES_OF_COLLISION * TYPES_OF_COLLISION> {
			nullptr, nullptr,
			nullptr, nullptr
		}
	} {
		int sphereIdx{ static_cast<int>(SPHERE) };
		int AABBIdx{ static_cast<int>(AABB) };
		int OBBIdx{ static_cast<int>(OBB) };

		/*Fill in the table of collisions, e.g collideFuncts[0] will be SPHERE TO SPHERE collision (2 total collider types will have 2^2 permutations(different types of collisions possible) ) */
		collideFuncts[sphereIdx] = SphereToSphereCollision;
		collideFuncts[AABBIdx * TYPES_OF_COLLISION + AABBIdx] = AABBToAABBCollision;
		collideFuncts[sphereIdx * TYPES_OF_COLLISION + AABBIdx] = SphereToAABBCollision;
		collideFuncts[AABBIdx * TYPES_OF_COLLISION + OBBIdx] = AABBToOBBCollision;
		collideFuncts[sphereIdx * TYPES_OF_COLLISION + OBBIdx] = SphereToOBBCollision;
		collideFuncts[OBBIdx * TYPES_OF_COLLISION + OBBIdx] = OBBToOBBCollision;
	}

	void CollisionHandler::update([[maybe_unused]] double deltaTime) {
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		std::vector<CrashoutsEngine::Entity> globalEntities = globalECS.GetComponentArray<Component::NameComponent>().indexToEntity;

		std::vector<std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity>> alreadyColl{};
		//std::vector<std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity>> test{};

		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : globalEntities) {

			if (!globalECS.HasComponent<Component::ColliderComponent>(entity1) || !globalECS.HasComponent<Component::TransformComponent>(entity1) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity1)) continue;
			for (CrashoutsEngine::Entity const& entity2 : globalEntities) {
				if (entity1 == entity2) continue;
				if (!globalECS.HasComponent<Component::ColliderComponent>(entity2) || !globalECS.HasComponent<Component::TransformComponent>(entity2) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity2)) continue;

				CrashoutsEngine::Entity const* entity1Ptr{ &entity1 };
				CrashoutsEngine::Entity const* entity2Ptr{ &entity2 };



				Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
				Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };
				Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(entity1) };

				Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(entity2) };
				Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(entity2) };
				//Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(entity2) };

				/*CHECK FOR COLLISION and call collision function*/
				Collision::CollisionManifold collisionManifold{};

				Component::TransformComponent newTransform1{ *transform1 };
				Component::RigidBodyComponent newRigidBody1{ *rigidBody1 };


				collisionManifold = callCollFunct(collider1->collider->type, collider2->collider->type)(
					Collision::CollisionProps{
						collider1->collider,
						CEVec3{},
						CEVec3{transform1->position}
					},
					Collision::CollisionProps{
						collider2->collider,
						CEVec3{},
						CEVec3{transform2->position}
					}
					);

				if (collisionManifold.collisionOccured) {

					if (collider1->collider->type > collider2->collider->type) {
						std::swap(entity1Ptr, entity2Ptr);
					}
					CollisionMessage colMessage{ Messaging::getCollisionMsgID(), collisionManifold, *entity1Ptr, *entity2Ptr };

					bool isIn{ false };

					if (*entity1Ptr > *entity2Ptr) std::swap(entity1Ptr, entity2Ptr);

					for (std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> test : alreadyColl) {
						if (test.first == *entity1Ptr && test.second == *entity2Ptr) isIn = true;
					}

					if (!isIn) {
						alreadyColl.push_back({ *entity1Ptr, *entity2Ptr });
						sendMessage(colMessage);
					}

				}

			}
		}

	}

	CollisionManifold CollisionHandler::OBBToOBBCollision([[maybe_unused]] CollisionProps const& coll1, [[maybe_unused]] CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };


		CollisionProps const* obbPtr1{ &coll1 };
		CollisionProps const* obbPtr2{ &coll2 };

		OBBRect const& obbTemp1 = dynamic_cast<OBBRect const&>(*(obbPtr1->collider));
		OBBRect const& obbTemp2 = dynamic_cast<OBBRect const&>(*(obbPtr2->collider));

		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp1.rotatedVertices, obbTemp1.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(obbTemp2.rotatedVertices, obbTemp1.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = obbTemp1.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = -1.f * obbTemp1.normalEdges[i];
				}
			}
		}

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp1.rotatedVertices, obbTemp2.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(obbTemp2.rotatedVertices, obbTemp2.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = obbTemp2.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = -1.f * obbTemp2.normalEdges[i];
				}
			}
		}

		result.depth = depthAmount;
		result.normal = smallestAxis;


		return result;
	}

	CollisionManifold CollisionHandler::SphereToSphereCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result;

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(coll1.collider));

		Sphere const& sphere2Temp = dynamic_cast<Sphere const&>(*(coll2.collider));

		float totalRadius{ sphere1Temp.radius + sphere2Temp.radius };
		CEVec3 circ1ToCirc2{ coll2.position - coll1.position };
		if (CEMath::LengthSq(circ1ToCirc2) - (totalRadius * totalRadius) > 0.f) {
			result.collisionOccured = false;
			return result;
		}

		/*HALF OF ACTUAL DEPTH TO ACCOUNT FOR THE TWO BODIES*/
		float depth{ std::abs(CEMath::Length(circ1ToCirc2) - totalRadius) * 0.5f };

		CEVec3 normal{ CEMath::Normalize(circ1ToCirc2) };
		float distanceToPoint{ sphere1Temp.radius - depth };
		CEVec3 contactPoint{ (normal * distanceToPoint) + coll1.position };

		result.depth = depth;
		result.normal = normal;
		result.contactPoints[0] = contactPoint;
		result.collisionOccured = true;

		return result;
	}

	CollisionManifold CollisionHandler::AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result;

		AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*(coll1.collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(coll2.collider));

		CEVec3 posDelta{ coll2.position - coll1.position }; /*Vector showing distance + the direction*/

		f32 xDiff{ aabb1Temp.extents.x + aabb2Temp.extents.x - std::abs(posDelta.x) };

		if (xDiff > 0.f) {
			f32 yDiff{ aabb1Temp.extents.y + aabb2Temp.extents.y - std::abs(posDelta.y) };

			if (yDiff > 0.f) {

				if (xDiff < yDiff) {
					result.collisionOccured = true;
					result.normal = posDelta.x < 0 ? CEVec3{ -1.f, 0.f, 0.f } : CEVec3{ 1.f, 0.f, 0.f };
					result.depth = xDiff;

				}
				else {
					result.collisionOccured = true;
					result.normal = posDelta.y < 0 ? CEVec3{ 0.f, -1.f, 0.f } : CEVec3{ 0.f, 1.f, 0.f };
					result.depth = yDiff;
				}

			}
		}
		return result;
	}

	CollisionManifold CollisionHandler::SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{};

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* aabbPtr{ &coll2 };
		if (spherePtr->collider->type > aabbPtr->collider->type) {
			std::swap(spherePtr, aabbPtr);
		}

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(spherePtr->collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));

		CEVec3 minAABB{ aabbPtr->position.x - aabb2Temp.extents.x, aabbPtr->position.y - aabb2Temp.extents.y, 0.f };
		CEVec3 maxAABB{ aabbPtr->position.x + aabb2Temp.extents.x, aabbPtr->position.y + aabb2Temp.extents.y, 0.f };


		CEVec3 pos{ spherePtr->position };

		CEVec3 closestPt{ pos }; /*CLOSEST POINT OF THE CENTER OF THE CIRCLE TO THE EDGES OF THE AABB*/

		/*CLAMP THE POINT OF THE CENTER OF THE CIRCLE TO THE POINT ON THE SIDE OF AABB*/
		if (pos.x < minAABB.x) { closestPt.x = minAABB.x; }
		if (pos.x > maxAABB.x) { closestPt.x = maxAABB.x; }

		if (pos.y < minAABB.y) { closestPt.y = minAABB.y; }
		if (pos.y > maxAABB.y) { closestPt.y = maxAABB.y; }

		CEVec3 ClosestPtToCirc{ closestPt - pos };
		float closestPtSquared{ CEMath::LengthSq(ClosestPtToCirc) };


		if (closestPtSquared < sphere1Temp.radius * sphere1Temp.radius) {


			result.collisionOccured = true;

			/*Center of Circle is already inside AABB*/
			if (closestPtSquared == 0.f) {
				CEVec3 vecFromAABBToCirc{ spherePtr->position - aabbPtr->position };

				float depthX{ sphere1Temp.radius + aabb2Temp.extents.x - fabsf(vecFromAABBToCirc.x) };

				float depthY{ sphere1Temp.radius + aabb2Temp.extents.y - fabsf(vecFromAABBToCirc.y) };

				if (depthX < depthY) {
					result.depth = depthX;
					result.normal = vecFromAABBToCirc.x < 0.f ? CEVec3{ -1.f, 0.f, 0.f } : CEVec3{ 1.f, 0.f, 0.f };

				}
				else {
					result.depth = depthY;
					result.normal = vecFromAABBToCirc.y < 0.f ? CEVec3{ 0.f, -1.f, 0.f } : CEVec3{ 0.f, 1.f, 0.f };
				}


			}
			else {
				f32 distance{ CEMath::Length(ClosestPtToCirc) };

				result.depth = sphere1Temp.radius - distance;
				result.normal = CEMath::Normalize(ClosestPtToCirc);
			}

		}

		return result;
	}

	CollisionManifold CollisionHandler::SphereToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* obbPtr{ &coll2 };
		if (spherePtr->collider->type > obbPtr->collider->type) {
			std::swap(spherePtr, obbPtr);
		}

		Sphere const& sphereTemp = dynamic_cast<Sphere const&>(*(spherePtr->collider));
		OBBRect const& obbTemp = dynamic_cast<OBBRect const&>(*(obbPtr->collider));

		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp.rotatedVertices, obbTemp.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projectCircleOntoAxis(sphereTemp, spherePtr->position, obbTemp.normalEdges[i]) };

			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };

			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * obbTemp.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = obbTemp.normalEdges[i];
				}
			}
		}

		CEVec3 const* closestPt{ findClosestVertex(obbTemp.rotatedVertices, spherePtr->position) };

		CEVec3 circleCenterToPtAxis{ *closestPt - spherePtr->position };
		circleCenterToPtAxis = CEMath::Normalize(circleCenterToPtAxis);

		std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp.rotatedVertices, circleCenterToPtAxis) };
		std::pair<f32, f32> projection2{ projectCircleOntoAxis(sphereTemp, spherePtr->position, circleCenterToPtAxis) };

		f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };

		if (overlap < 0.f) {
			result.collisionOccured = false;
		}

		if (overlap < depthAmount) {
			depthAmount = overlap;
			smallestAxis = -1.f * circleCenterToPtAxis;
			if (projection1.second > projection2.second) {
				smallestAxis = circleCenterToPtAxis;
			}
		}


		result.depth = depthAmount;
		result.normal = smallestAxis;


		return result;
	}

	CollisionManifold CollisionHandler::AABBToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };

		CollisionProps const* aabbPtr{ &coll1 };
		CollisionProps const* obbPtr{ &coll2 };
		if (aabbPtr->collider->type > obbPtr->collider->type) {
			std::swap(obbPtr, aabbPtr);
		}

		AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));
		OBBRect const& obb2Temp = dynamic_cast<OBBRect const&>(*(obbPtr->collider));

		std::array<CEVec3, 4> aabbVertex{
			CEVec3{aabb1Temp.min.x, aabb1Temp.min.y, 0.0f },
			{aabb1Temp.min.x, aabb1Temp.min.y + aabb1Temp.extents.y * 2.f, 0.0f },
			CEVec3{aabb1Temp.max.x, aabb1Temp.max.y, 0.0f },
			{aabb1Temp.min.x + aabb1Temp.extents.x * 2.f, aabb1Temp.min.y, 0.0f}
		};

		std::array<CEVec3, 2> aabbNormals{
			CEVec3{-1.f, 0.f, 0.0f},
			CEVec3{0.f, 1.f, 0.0f}
		};


		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obb2Temp.rotatedVertices, obb2Temp.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(aabbVertex, obb2Temp.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * obb2Temp.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = 1.f * obb2Temp.normalEdges[i];
				}
			}
		}

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obb2Temp.rotatedVertices, aabbNormals[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(aabbVertex, aabbNormals[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * aabbNormals[i];
				if (projection1.second > projection2.second) {
					smallestAxis = 1.f * aabbNormals[i];
				}
			}
		}

		result.depth = depthAmount;
		result.normal = smallestAxis;

		return result;
	}



	FunctionDispatchType CollisionHandler::callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2) {

		/*Swap the type according to enum order, e.g function exists for sphere to aabb but not aabb to sphere*/
		if (type1 > type2) {
			std::swap(type1, type2);
		}

		int idx1{ static_cast<int>(type1) };
		int idx2{ static_cast<int>(type2) };

		return collideFuncts[idx1 * TYPES_OF_COLLISION + idx2];
	}

	/*----------------------------------------------------------------------------------------------------------*/
}