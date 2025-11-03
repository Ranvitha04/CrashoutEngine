/*                                                   Includes
----------------------------------------------------------------------------- */
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
#include "physicsmanager.h"
#include <random>
#include <string>
#include <chrono> // Used to seed the random number generator

#include "scriptcomponent.h"
#include "glm/vec3.hpp"
#include "GLFW/glfw3.h"



float random_min_max(float const& min, float const& max) {
	// 1. Setup the Random Number Generator (RNG)

	// Use a high-quality entropy source (like std::random_device) to seed the engine
	// A better practice is often seeding once at the start of your program.
	static std::random_device rd;

	// Use a Mersenne Twister engine for high-quality pseudo-random numbers
	static std::mt19937 generator(rd());

	// 2. Define the Distribution

	// Create a distribution for floating-point numbers between 0.0 and 255.0 (inclusive)
	std::uniform_real_distribution<float> distribution(min, max);

	// 3. Generate and Return the Number

	// Pass the generator to the distribution to get the next random number
	return distribution(generator);
}
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;

static int itemCount = 0;

static CrashoutsEngine::Entity entity1;
static CrashoutsEngine::Entity entity2;
static CrashoutsEngine::Entity entity3;

void AddPhysicsEntity(CrashoutsEngine::Entity &entity, Component::RigidBodyComponent&& rigidBody, 
	Component::TransformComponent&& transform, Component::ShapeComponent &&shape) {

	entity = globalECS->CreateEntity();

	CEVec3 extents{ shape.width * 0.5f, shape.height * 0.5f, 0.f };

	globalECS->AddComponent<Component::NameComponent>(entity, Component::NameComponent{std::to_string(entity)});
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

void LoadMainMenu()
{
	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	using namespace CrashoutsEngine::Assets;
	AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();

	gTextRenderer = std::make_unique<TextRenderer>();
	gTextRendererA = std::make_unique<TextRenderer>();
}

void InitMainMenu()
{
	AddPhysicsEntity(
		entity1,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .position = CEVec3{0.f, 20.f, 0.f} },
		Component::ShapeComponent{.enableTexture=true, .textureName="Concept", .width = 10.f, .height = 10.f}
	);
	globalECS->AddComponent<CrashoutsEngine::ScriptComponent>(entity1, CrashoutsEngine::ScriptComponent("PlayerController", entity1));

	AddPhysicsEntity(
		entity2,
		Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f },
		Component::TransformComponent{ .position = CEVec3{0.f, -10.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle, .width = 80.f, .height = 10.f }
	);
	

	AddPhysicsEntity(
		entity3,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .position = CEVec3{5.f, 20.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle, .width = 10.f, .height = 10.f }
	);
	globalECS->AddComponent<CrashoutsEngine::ScriptComponent>(entity3, CrashoutsEngine::ScriptComponent("EnemyController", entity3));
	// Initialize MainMenu state

	LOGI(Core, "MainMenu Initialized");
}

void CreateRandBox() {
	int entityId = globalECS->CreateEntity();
	globalECS->AddComponent<Component::NameComponent>(entityId, Component::NameComponent{ std::to_string(entityId) });

	Component::TransformComponent t;
	Component::ShapeComponent shape;
	// Set random pos rot scale
	float width = 100.f;
	float height = 50.f;
	t.position = CEVec3(random_min_max(-width, width), random_min_max(-height, height), 0.f);
	t.rotation = random_min_max(0.f, 360.f);
	t.size = CEVec2(1.f, 1.f);

	shape.width = random_min_max(.5f, 3.f);
	shape.height = random_min_max(.5f, 3.f);
	shape.size = random_min_max(.5f, 3.f);
	shape.color = CEVec3(random_min_max(0.f, 255.f), random_min_max(0.f, 255.f), random_min_max(0.f, 255.f));
	shape.type = Component::ShapeType::Square;


	globalECS->AddComponent<Component::TransformComponent>(entityId, t);
	globalECS->AddComponent<Component::ShapeComponent>(entityId, shape);
	
	if (rand() % 2) {
		CEVec3 rb_init_velocity{ 0.f, 0.f, 0.f };
		float rb_mass = .5f;
		float rb_restitution = 0.f;
		globalECS->AddComponent<Component::RigidBodyComponent>(entityId, rb_init_velocity, rb_mass, rb_restitution);
	}
}

void UpdateMainMenu()
{

	//Component::RigidBodyComponent* rigidBody{ globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };
	//Component::TransformComponent* transform{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

	//Component::RigidBodyComponent* rigidBody2{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };

	if (itemCount < 2500 && InputManager::IsKeyPressed(Key::Num1)) {
		int newCount = 0;
		while (newCount < 1) {
			CreateRandBox();
			newCount++;
			itemCount++;
		}
	} 

	/*
	if (rigidBody) {
		if (InputManager::IsKeyPressed(Key::A)) rigidBody->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::D)) rigidBody->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::W)) rigidBody->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody->getMass());
	}
	*/
	/*
	if (rigidBody2) {
		if (InputManager::IsKeyPressed(Key::Left)) rigidBody2->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Right)) rigidBody2->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Up)) rigidBody2->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody2->getMass());
	}
	*/
	/*
	// SCALE UP
	if (InputManager::IsKeyPressed(Key::S)) {
		transform->size *= 0.9f;
	}

	// SCALE DOWN 
	if (InputManager::IsKeyPressed(Key::Q)) {
		transform->size *= 1.1f;
	}
	
	// ROTATE
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{ globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type == Collision::ColliderType::OBB) {
				Collision::OBBRect & obbTemp2 = dynamic_cast<Collision::OBBRect &>(*(collider1->collider));
				obbTemp2.updateRotation(transform->rotation);
			}
		}
	}*/
}

void RenderMainMenu()
{
	// Render MainMenu state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Scene::RenderScene(*globalECS);
}

void FreeMainMenu()
{
	// Free resources specific to MainMenu state
	globalECS->DestroyEntity(entity1);
	globalECS->DestroyEntity(entity2);
	globalECS->DestroyEntity(entity3);
}

void UnloadMainMenu()
{
	// Unload MainMenu state
}