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

static void AddPhysicsEntity(CrashoutsEngine::Entity& entity, Component::RigidBodyComponent&& rigidBody,
	Component::TransformComponent&& transform, Component::ShapeComponent&& shape) {

	entity = globalECS->CreateEntity();

	CEVec3 extents{ transform.size.x * 0.5f, transform.size.y * 0.5f, 0.f };

	globalECS->AddComponent<Component::NameComponent>(entity, Component::NameComponent{ std::to_string(entity) });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity, rigidBody);
	globalECS->AddComponent<Component::TransformComponent>(entity, transform);
	globalECS->AddComponent<Component::ShapeComponent>(entity, shape);
	globalECS->AddComponent<Component::ColliderComponent>(entity, Component::ColliderComponent{
		std::make_unique<Collision::OBBRect>(Collision::OBBRect{
			CEVec3{transform.position - extents },
			CEVec3{transform.position + extents}
		})
		});
}

enum AI_STATES {
	PATROL_LEFT,
	PATROL_RIGHT
};

class PatrolRight : public State::StateWrapper<State::StateType> {
public:
	PatrolRight() : StateWrapper{ static_cast<State::StateType>(PATROL_RIGHT) } {

		enter = []() {};

		update = []() {
			Component::RigidBodyComponent* comp{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };
			Component::AIComponent* fsm{ globalECS->GetComponent<Component::AIComponent>(entity3) };

			if (!AIUtils::IsPlatformInFront(entity3, 1.f)) {
				fsm->fsm.SetCurrentState(PATROL_LEFT);
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, 0.f, 0.f });
				return;
			}
			comp->addForce(CEVec3{ 100.f, 0.f, 0.f });
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
			if (!AIUtils::IsPlatformInFront(entity3, -1.f)) {
				fsm->fsm.SetCurrentState(PATROL_RIGHT);
				comp->clearForceAccumulator();
				comp->setVelocity(CEVec3{ 0.f, 0.f, 0.f });
				return;
			}
			comp->addForce(CEVec3{ -100.f, 0.f, 0.f });
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
		Component::TransformComponent{ .size = CEVec2{10.f, 10.f}, .position = CEVec3{0.f, 20.f, 0.f} },
		Component::ShapeComponent{ .enableTexture = true, .textureName = "Concept" }
	);

	AddPhysicsEntity(
		entity2,
		Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{80.f, 10.f}, .position = CEVec3{0.f, -10.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle }
	);

	AddPhysicsEntity(
		entity3,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{10.f, 10.f}, .position = CEVec3{5.f, 20.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle }
	);

	State::FiniteStateMachine<State::StateType> fsm{};

	fsm.Add(PatrolRight{});
	fsm.Add(PatrolLeft{});
	//fsm.Add(PatrolLeft{});

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
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	using namespace CrashoutsEngine;

	Scene::RenderScene(*globalECS);


}

void FreeTristan() {
	globalECS->Reset();
}

void UnloadTristan()
{

}