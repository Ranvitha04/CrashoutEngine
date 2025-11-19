#include "mainmenuscene.h"

#include "sceneutils.h"
#include "textrenderer.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include "inputmanager.h"
#include "collisionmanager.h"
#include "aicomponent.h"
#include "CEMath.h"
#include "frameratecontroller.h"
#include "collidereventmanager.h"
#include "shapescomponent.h"
#include "namecomponent.h"
#include "physicsmanager.h"
#include "aimanager.h"
#include <iostream>
#include <random>
#include <string>
#include <chrono> // Used to seed the random number generator
#include "glm/vec3.hpp"
#include "GLFW/glfw3.h"


static CrashoutsEngine::ECS* globalECS;

static CrashoutsEngine::Entity entity1;
static CrashoutsEngine::Entity entity2;
static CrashoutsEngine::Entity entity3;
static CrashoutsEngine::Entity entity4;

static void AddPhysicsEntity(CrashoutsEngine::Entity& entity, Component::RigidBodyComponent&& rigidBody,
	Component::TransformComponent&& transform, Component::ShapeComponent&& shape, Component::Tag&& name) {

	entity = globalECS->CreateEntity();

	CEVec3 extents{ transform.size.x * 0.5f, transform.size.y * 0.5f, 0.f };

	globalECS->AddComponent<Component::NameComponent>(entity, Component::NameComponent{ std::to_string(entity) });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity, rigidBody);
	globalECS->AddComponent<Component::TransformComponent>(entity, transform);
	globalECS->AddComponent<Component::ShapeComponent>(entity, shape);
	globalECS->AddComponent<Component::Tag>(entity, name);
	globalECS->AddComponent<Component::ColliderComponent>(entity, Component::ColliderComponent{
		std::make_unique<Collision::Polygon>(Collision::Polygon::createOBB(transform.position + extents, transform.position - extents, 50.f, CEVec3{0.f, 0.f, 0.f}))
	});

	globalECS->GetComponent<Component::RigidBodyComponent>(entity)->setInertia(200.f);

}

static void AddPhysicsEntity2(CrashoutsEngine::Entity& entity, Component::RigidBodyComponent&& rigidBody,
	Component::TransformComponent&& transform, Component::ShapeComponent&& shape, Component::Tag&& name) {

	entity = globalECS->CreateEntity();

	CEVec3 extents{ transform.size.x * 0.5f, transform.size.y * 0.5f, 0.f };

	globalECS->AddComponent<Component::NameComponent>(entity, Component::NameComponent{ std::to_string(entity) });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity, rigidBody);
	globalECS->AddComponent<Component::TransformComponent>(entity, transform);
	globalECS->AddComponent<Component::ShapeComponent>(entity, shape);
	globalECS->AddComponent<Component::Tag>(entity, name);
	globalECS->AddComponent<Component::ColliderComponent>(entity, Component::ColliderComponent{
		std::make_unique<Collision::Sphere>(Collision::Sphere{10.f, {transform.position.x, transform.position.y, 0.f}}) });

	globalECS->GetComponent<Component::RigidBodyComponent>(entity)->setInertia(globalECS->GetComponent<Component::ColliderComponent>(entity)->collider->calculateInertia(globalECS->GetComponent<Component::RigidBodyComponent>(entity)->getMass()));
	
}

static void AddObstacleEntity(CEVec3&& transform, CEVec2&& dimen, std::string&& tag) {
	CrashoutsEngine::Entity e;

	AddPhysicsEntity(
		e,
		Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f },
		Component::TransformComponent{ .size = dimen, .position = transform },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle },
		Component::Tag{ tag }
	);
}

static void AddPlatformEntity(CEVec3&& transform, CEVec2&& dimen) {
	AddObstacleEntity(std::move(transform), std::move(dimen), "P");
}

static void AddWallEntity(CEVec3&& transform, CEVec2&& dimen) {
	AddObstacleEntity(std::move(transform), std::move(dimen), "W");
}

enum AI_STATES {
	PATROL_LEFT,
	PATROL_RIGHT,
	STOP,
	CHASE,
	JUMP
};

class PatrolRight : public State::StateWrapper<State::StateType> {
public:
	PatrolRight() : StateWrapper{ static_cast<State::StateType>(PATROL_RIGHT) } {

		enter = []() {};

		update = []() {
			Component::RigidBodyComponent* comp{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };
			Component::AIComponent* fsm{ globalECS->GetComponent<Component::AIComponent>(entity3) };
			CEVec3 origVel{ comp->getVelocity() };
			if (!AIUtils::IsPlatformInFront(entity3, 1.f) || AIUtils::IsObstacleInFront(entity3, 1.f)) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(PATROL_LEFT);
				return;
			} if (AIUtils::IsIntersecting(entity1, entity3)) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(STOP);
				return;
			}
			comp->addForce(CEVec3{ 100.f, 0.f, 0.f });
			};

		exit = []() {};

	}
};

class Chase : public State::StateWrapper<State::StateType> {
public:
	Chase() : StateWrapper{ static_cast<State::StateType>(CHASE) } {

		enter = []() {};

		update = []() {
			Component::RigidBodyComponent* comp{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };
			Component::AIComponent* fsm{ globalECS->GetComponent<Component::AIComponent>(entity3) };

			Component::TransformComponent* t1{ globalECS->GetComponent<Component::TransformComponent>(entity3) };
			Component::TransformComponent* t2{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

			CEVec3 origVel{ comp->getVelocity() };

			if (AIUtils::IsIntersecting(entity1, entity3)) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(STOP);
				return;
			}

			if (!AIUtils::EntityInRange(entity1, entity3, 40.f)) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(PATROL_RIGHT);
				return;
			}

			CEVec3 diffVect{ t2->position - t1->position };
			f32 dir{ CEMath::Dot(CEVec3{1.f, 0.f, 0.f}, diffVect) };
			CEVec3 relVector{ AIUtils::sign(dir) * 100.f, 0.f, 0.f };

			if (AIUtils::IsObstacleIntersecting(entity3)) {
				relVector.y += 500.f;
			}
			comp->addForce(CEVec3{ relVector });
			};

		exit = []() {};

	}
};

class PatrolLeft : public State::StateWrapper<State::StateType> {
public:
	PatrolLeft() : StateWrapper{ static_cast<State::StateType>(PATROL_LEFT) } {
		enter = []() {};

		update = []() {
			Component::RigidBodyComponent* comp{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };
			Component::AIComponent* fsm{ globalECS->GetComponent<Component::AIComponent>(entity3) };
			CEVec3 origVel{ comp->getVelocity() };
			if (AIUtils::EntityInRange(entity1, entity3, 40.f)) {
				fsm->fsm.SetCurrentState(CHASE);
				return;
			}
			if (!AIUtils::IsPlatformInFront(entity3, -1.f, std::string{ "P" }) || AIUtils::IsObstacleInFront(entity3, -1.f, std::string{ "W" })) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(PATROL_RIGHT);
				return;
			}
			if (AIUtils::IsIntersecting(entity1, entity3)) {
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, origVel.y, 0.f });
				fsm->fsm.SetCurrentState(STOP);
				return;
			}
			comp->addForce(CEVec3{ -100.f, 0.f, 0.f });
			};



		exit = []() {};
	}
};

class Stop : public State::StateWrapper<State::StateType> {
public:
	Stop() : StateWrapper{ static_cast<State::StateType>(STOP) } {
		enter = []() {};

		update = []() {
			Component::AIComponent* fsm{ globalECS->GetComponent<Component::AIComponent>(entity3) };
			if (AIUtils::EntityInRange(entity1, entity3, 40.f)) {
				fsm->fsm.SetCurrentState(CHASE);
				return;
			}
			else {
				fsm->fsm.SetCurrentState(PATROL_RIGHT); //PLACEHOLDER
			}
			};

		exit = []() {};
	}
};

void Enter([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {

	Component::Tag* test{ globalECS->GetComponent<Component::Tag>(collidedEntity) };
	Component::ShapeComponent* test2{ globalECS->GetComponent<Component::ShapeComponent>(collidedEntity) };

	if (test->id == std::string{ "W" }) test2->color = CEVec3{ 255.f, 255.f, 0.f };
}

void Stay([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity1) };

	test->color = CEVec3{ 255.f, 0.f, 0.f };
}

void Exit([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity1) };

	test->color = CEVec3{ 0.f, 0.f, 255.f };
}


void Enter2([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity1) };

	test->color = CEVec3{ 0.f, 255.f, 0.f };
}

void Stay2([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity3) };

	test->color = CEVec3{ 255.f, 0.f, 0.f };
}

void Exit2([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity3) };

	test->color = CEVec3{ 0.f, 0.f, 255.f };
}

void Enter3([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity4) };

	test->color = CEVec3{ 255.f, 255.f, 0.f };
}

void Stay3([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity4) };

	test->color = CEVec3{ 255.f, 0.f, 0.f };
}

void Exit3([[maybe_unused]] CrashoutsEngine::Entity collidedEntity) {
	Component::ShapeComponent* test{ globalECS->GetComponent<Component::ShapeComponent>(entity4) };

	test->color = CEVec3{ 0.f, 0.f, 255.f };
}

void LoadTristan() {
	LOGI(Core, "Tristan Initialized");

	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

}


void InitTristan() {
	std::cout << "INIT";

	AddPhysicsEntity(
		entity1,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{5.f, 5.f}, .position = CEVec3{-20.f, 10.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle },
		Component::Tag{ "A" }
	);

	AddPlatformEntity(CEVec3{}, CEVec2{ 200.f, 5.f });
	AddWallEntity(CEVec3{ 20.f, 5.f, 0.0f }, CEVec2{ 10.f, 5.f });

	AddWallEntity(CEVec3{ 40.f, 5.f, 0.0f }, CEVec2{ 10.f, 10.f });

	AddPhysicsEntity(
		entity3,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{5.f, 5.f}, .position = CEVec3{5.f, 20.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle },
		Component::Tag{ "B" }
	);

	AddPhysicsEntity(
		entity4,
		Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{5.f, 5.f}, .position = CEVec3{5.f, 20.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle },
		Component::Tag{ "B" }
	);


	/*State::FiniteStateMachine<State::StateType> fsm{};

	fsm.Add(PatrolRight{});
	fsm.Add(PatrolLeft{});
	fsm.Add(Stop{});
	fsm.Add(Chase{});

	fsm.SetCurrentState(PATROL_RIGHT);

	globalECS->AddComponent<Component::AIComponent>(entity3, Component::AIComponent{
		fsm
	});*/

	CrashoutsEngine::CollisionEventManager& colEvent{ CrashoutsEngine::CollisionEventManager::GetInstance() };

	colEvent.registerCollideStay(entity3, Stay2);
	colEvent.registerCollideExit(entity3, Exit2);
	colEvent.registerCollideEnter(entity3, Enter2);

	colEvent.registerCollideStay(entity1, Stay);
	colEvent.registerCollideExit(entity1, Exit);
	colEvent.registerCollideEnter(entity1, Enter);

	colEvent.registerTriggerStay(entity4, Stay3);
	colEvent.registerTriggerExit(entity4, Exit3);
	colEvent.registerTriggerEnter(entity4, Enter3);



}

void UpdateTristan()
{
	Component::RigidBodyComponent* rigidBody{ globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };
	Component::TransformComponent* transform{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

	//Component::RigidBodyComponent* rigidBody2{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };

	f32 dt{ f32(FrameRateController::GetInstance().DT()) };
	if (rigidBody) {
		if (InputManager::IsKeyPressed(Key::A)) rigidBody->setVelocity(CEVec3{-15.f, rigidBody->getVelocity().y, 0.f});
		else if (InputManager::IsKeyPressed(Key::D)) rigidBody->setVelocity(CEVec3{15.f, rigidBody->getVelocity().y, 0.f });
		else {
			rigidBody->setVelocity(CEVec3{ 0.f, rigidBody->getVelocity().y, 0.f });
		}

		if (InputManager::IsKeyPressed(Key::Space)) rigidBody->addForce(CEVec3{ 0.f, 4000.f, 0.f }*40.f *dt );
	}

	/*ROTATE*/
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{ globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type == Collision::ColliderType::POLYGON) {
				Collision::Polygon& obbTemp2 = dynamic_cast<Collision::Polygon&>(*(collider1->collider));
				obbTemp2.updateRotation(transform->rotation);
			}
		}
	}


}

void RenderTristan() {
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	using namespace CrashoutsEngine;

	Scene::RenderScene(*globalECS);


}

void FreeTristan() {
	globalECS->Reset();
	CrashoutsEngine::CollisionEventManager& currentEvents{ CrashoutsEngine::CollisionEventManager::GetInstance() };

	currentEvents.getCollisionEvents().clear();
}

void UnloadTristan()
{

}