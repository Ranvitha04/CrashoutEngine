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
#include "physicsmanager.h"
#include "aimanager.h"
#include <iostream>
#include <random>
#include <string>
#include <chrono> // Used to seed the random number generator
#include "glm/vec3.hpp"
#include "GLFW/glfw3.h"
#include "collidercomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "namecomponent.h"

static CrashoutsEngine::ECS* globalECS;

static CrashoutsEngine::Entity entity1;
static CrashoutsEngine::Entity entity2;
static CrashoutsEngine::Entity entity3;

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
		std::make_unique<Collision::AABBRect>(Collision::AABBRect{
			CEVec3{transform.position - extents },
			CEVec3{transform.position + extents}
		})
		});
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


void LoadTristan() {
	LOGI(Core, "Tristan Initialized");

	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

}

void InitTristan() {
	AddPhysicsEntity(
		entity1,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{5.f, 5.f}, .position = CEVec3{-20.f, 10.f, 0.f} },
		Component::ShapeComponent{ .enableTexture = true, .textureName = "Concept" },
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


	State::FiniteStateMachine<State::StateType> fsm{};

	fsm.Add(PatrolRight{});
	fsm.Add(PatrolLeft{});
	fsm.Add(Stop{});
	fsm.Add(Chase{});

	fsm.SetCurrentState(PATROL_RIGHT);

	globalECS->AddComponent<Component::AIComponent>(entity3, Component::AIComponent{
		fsm
		});

}

void UpdateTristan()
{
	Component::RigidBodyComponent* rigidBody{ globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };
	Component::TransformComponent* transform{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

	//Component::RigidBodyComponent* rigidBody2{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };


	if (rigidBody) {
		if (InputManager::IsKeyPressed(Key::A)) rigidBody->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::D)) rigidBody->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::W)) rigidBody->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody->getMass());
	}

	/*ROTATE*/
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{ globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type == Collision::ColliderType::OBB) {
				Collision::OBBRect& obbTemp2 = dynamic_cast<Collision::OBBRect&>(*(collider1->collider));
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
}

void UnloadTristan()
{

}