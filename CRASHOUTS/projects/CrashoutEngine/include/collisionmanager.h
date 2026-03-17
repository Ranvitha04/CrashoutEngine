/*!
@file       collisionmanager.h
@author     Renfred Lee
@author     tristan
@author     Shahrin Idzuan
@date       08/09/2025
@brief      Basic Collision Functions and Colliders(Plane, ColliderType::AABB and Circle)

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include "transformcomponent.h"
#include "observable.h"
#include "log.h"
#include "colliders.h"
#include "collisionutils.h"
#include "entitymanager.h"
#include "collidercomponent.h"
#include "quadtree.h"
#include "messages.h"
#include <memory>
#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <utility>
#include <unordered_set>

namespace Collision {

    enum class EngineState { STOP, PLAY, PAUSE };

    /*Custom Types*/
    enum TypeOfCollision {
        ON_ENTER,
        ON_STAY,
        ON_EXIT,
        COUNT
    };

    /*!
    @class    CollisionMessage
    @brief    Used as the data to collision messages
    */
    class CollisionMessage : public Messaging::Messages {
    private:
        CollisionManifold colManifold;
        CrashoutsEngine::Entity entity1;
        CrashoutsEngine::Entity entity2;
    public:
        CollisionMessage(const Messaging::EventType& type, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2);
        std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> GetCollisionPair();
        CollisionManifold GetCollisionManifold();
    };

    /*!
    @struct    CollisionProps
    @brief    Used to pass data about entities(collider, velocity and position)
    */
    struct CollisionProps {
        std::unique_ptr<Collider> const& collider;
        const CEVec3& vel;
        const CEVec3& position;
    };

    using FunctionDispatchType = CollisionManifold(*)(CollisionProps const& coll1, CollisionProps const& coll2);

    /*!
    @class    CollisionHandler
    @brief    Used to contain functions for checking collision and able to return a function pointer based on the type of collision passed in
    */
    class CollisionHandler : public Messaging::Observable {
    public:
        using Contact = std::optional<CollisionManifold>;

    private:

        static constexpr int SHAPE_COUNT = static_cast<int>(Collision::ColliderType::NONE);

        std::array<FunctionDispatchType, SHAPE_COUNT* SHAPE_COUNT> collideFuncts;

        static CollisionManifold CircleToCircleCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold CircleToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

        /*ColliderType::POLYGON FUNCTIONS*/
        static CollisionManifold PolygonToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold CircleToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold AABBToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2);

        /*ColliderType::LINE FUNCTIONS*/
        static CollisionManifold CircleToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold PolygonToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold AABBToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2);
        static CollisionManifold LineToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2);

        std::vector<CollisionEvent> m_FrameCollisions;

    public:

        ContactCollection currentFrameCollision;
        ContactCollection lastFrameCollision;

        static CollisionHandler& GetInstance() {
            static CollisionHandler singleton;
            return singleton;
        }

        // --- DUAL QUADTREE SYSTEM ---
        DataStructure::QuadTree staticQuadTree;
        DataStructure::QuadTree dynamicQuadTree;
        size_t lastStaticCount = 0;
        bool isStaticDirty = true;

        // Helper to query both trees at once
        void QueryTrees(AABBRect& region, std::vector<CrashoutsEngine::Entity>& outCandidates) {
            // 1. Query static objects (this might internally clear outCandidates, which is fine here)
            staticQuadTree.Query(region, outCandidates);

            // 2. Query dynamic objects into a temporary list so they don't overwrite the static ones
            std::vector<CrashoutsEngine::Entity> dynamicCands;
            dynamicCands.reserve(32);
            dynamicQuadTree.Query(region, dynamicCands);

            // 3. Combine them!
            outCandidates.insert(outCandidates.end(), dynamicCands.begin(), dynamicCands.end());
        }

        CollisionHandler();

        void Update(double deltaTime, EngineState engineState, int subStepIndex);

        /*!
        @brief Return a function pointer to call the appropriate collision function
        */
        FunctionDispatchType callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2);

        /*!
        @brief Cleanup
        */
        void Shutdown() {
            std::vector<CollisionEvent>().swap(m_FrameCollisions);

            ContactCollection().swap(currentFrameCollision);
            ContactCollection().swap(lastFrameCollision);

            staticQuadTree.Clear();
            dynamicQuadTree.Clear();
            lastStaticCount = 0;
            isStaticDirty = true;

            ClearObservers();
        }

        // Getter for collision data that frame
        const std::vector<CollisionEvent>& GetFrameCollisions() const {
            return m_FrameCollisions;
        }

        bool Raycast(CEVec3 start, CEVec3 end, CrashoutsEngine::Entity& outEntity, CrashoutsEngine::Entity ignoredEntity = 999999);

        bool RaycastHit(CEVec3 start, CEVec3 end, CEVec3& outPoint, CrashoutsEngine::Entity& outEntity,
            CrashoutsEngine::Entity ignoredEntity = 999999, CrashoutsEngine::Entity ignoredParent = 0);

        bool RaycastAgainstList(CEVec3 start, CEVec3 end, const std::vector<CrashoutsEngine::Entity>& candidates, CEVec3& outPoint,
            CrashoutsEngine::Entity& outEntity, CrashoutsEngine::Entity ignoredEntity = 0, CrashoutsEngine::Entity ignoredParent = 0);
    };
}