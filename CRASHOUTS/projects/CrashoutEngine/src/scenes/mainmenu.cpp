/*                                                   Includes
----------------------------------------------------------------------------- */
#include "scenes/mainmenu.h"

#include "sceneutils.h"
#include "textrenderer.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
#include "managers/entitymanager.h"
#include "managers/assetmanager.h"
#include "managers/inputmanager.h"
#include "managers/collisionmanager.h"
#include "managers/physicsmanager.h"
#include <random>
#include <string>
#include <chrono> // Used to seed the random number generator
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
		std::make_unique<Collision::AABBRect>(Collision::AABBRect{
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

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();

	GLuint shaderID = loadShaderProgram("../../../projects/CrashoutEngine/shaders/textShader.vert", "../../../projects/CrashoutEngine/shaders/textShader.frag");
	if (shaderID == 0) {
		std::cout << "Failed to load text shader.\n";
		return;
	}
	gTextRenderer = new TextRenderer("../../../projects/CrashoutEngine/assets/fonts/arial.ttf", Settings::GetScreenWidth(), Settings::GetScreenHeight());
	gTextRendererA = new TextRenderer("../../../projects/CrashoutEngine/assets/fonts/Exo2-Regular.ttf", Settings::GetScreenWidth(), Settings::GetScreenHeight());
}

void InitMainMenu()
{
	AddPhysicsEntity(
		entity1,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .position = CEVec3{0.f, 20.f, 0.f} },
		Component::ShapeComponent{.enableTexture=true, .textureName="duck", .width = 10.f, .height = 10.f}
	);

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

	// Initialize MainMenu state
	LOGI(Core, "MainMenu Initialized");
}

void CreateThing() {
	Component::ShapeComponent shape;
	int entityId = globalECS->CreateEntity();
	Component::TransformComponent t;
	// Set random pos rot scale
	float width = 100.f;
	float height = 50.f;
	t.position = CEVec3(random_min_max(-width, width), random_min_max(-height, height), 0.f);
	t.rotation = random_min_max(0.f, 360.f);
	t.size = CEVec2(1.f, 1.f);

	shape.width = random_min_max(2.f, 3.f);
	shape.height = random_min_max(2.f, 5.f);
	shape.color = CEVec3(random_min_max(0.f, 255.f), random_min_max(0.f, 255.f), random_min_max(0.f, 255.f));
	shape.type = Component::ShapeType::Rectangle;

	globalECS->AddComponent<Component::TransformComponent>(entityId, t);
	globalECS->AddComponent<Component::ShapeComponent>(entityId, shape);

}

void UpdateMainMenu()
{

	Component::RigidBodyComponent* rigidBody{ globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };
	Component::TransformComponent* transform{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

	Component::RigidBodyComponent* rigidBody2{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };

	if (itemCount < 2500 && InputManager::IsKeyPressed(Key::Num1)) {
		int newCount = 0;
		while (newCount < 10) {
			CreateThing();
			newCount++;
			itemCount++;
		}
	} 
	
	if (rigidBody) {
		if (InputManager::IsKeyPressed(Key::A)) rigidBody->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::D)) rigidBody->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::W)) rigidBody->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody->getMass());
	}

	if (rigidBody2) {
		if (InputManager::IsKeyPressed(Key::Left)) rigidBody2->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Right)) rigidBody2->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Up)) rigidBody2->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody2->getMass());
	}

	/*SCALE UP*/
	if (InputManager::IsKeyPressed(Key::S)) {
		transform->size *= 0.9f;
	}

	/*SCALE DOWN*/
	if (InputManager::IsKeyPressed(Key::Q)) {
		transform->size *= 1.1f;
	}

	/*ROTATE*/
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
	}
}

void RenderMainMenu()
{
	// Render MainMenu state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Scene::RenderScene(*globalECS);
	
	if (gTextRenderer) {
		gTextRenderer->renderCenteredText("Hello", -900.0f, 0.0f, 1.0f, glm::vec3(0.0f));
	}
	if (gTextRendererA) {
		gTextRendererA->renderCenteredText(std::to_string(itemCount), -900.0f, 100.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	}

}

void FreeMainMenu()
{
	// Free resources specific to MainMenu state
	delete gTextRenderer;
	gTextRenderer = nullptr;

	delete gTextRendererA;
	gTextRendererA = nullptr;

	globalECS->DestroyEntity(entity1);
	globalECS->DestroyEntity(entity2);
	globalECS->DestroyEntity(entity3);
}

void UnloadMainMenu()
{
	// Unload MainMenu state
}