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
#include <random>
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

void LoadMainMenu()
{
	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
	
	GLuint shaderID = LoadShaderProgram("../../../projects/CrashoutEngine/shaders/textShader.vert", "../../../projects/CrashoutEngine/shaders/textShader.frag");
	if (shaderID == 0) {
		std::cout << "Failed to load text shader.\n";
		return;
	}
	gTextRenderer = new TextRenderer("../../../projects/CrashoutEngine/assets/fonts/arial.ttf", Settings::GetScreenWidth(), Settings::GetScreenHeight());
	gTextRendererA = new TextRenderer("../../../projects/CrashoutEngine/assets/fonts/Exo2-Regular.ttf", Settings::GetScreenWidth(), Settings::GetScreenHeight()); 
}

void InitMainMenu()
{
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

	shape.width = random_min_max(5.f, 25.f);
	shape.height = random_min_max(5.f, 25.f);
	shape.color = CEVec3(random_min_max(0.f, 255.f), random_min_max(0.f, 255.f), random_min_max(0.f, 255.f));
	shape.type = Component::ShapeType::Rectangle;

	globalECS->AddComponent<Component::TransformComponent>(entityId, t);
	globalECS->AddComponent<Component::ShapeComponent>(entityId, shape);
}

void UpdateMainMenu()
{
	// Update logic for MainMenu state

	if (itemCount < 2500 && InputManager::IsKeyPressed(Key::S)) {
		int newCount = 0;
		while (newCount < 10) {
			CreateThing();
			newCount++;
			itemCount++;
		}
	}
}

void RenderMainMenu()
{
	// Render MainMenu state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Scene::RenderScene(*globalECS);
	
	if (gTextRenderer) {
		gTextRenderer->RenderCenteredText("Hello", -900.0f, 0.0f, 1.0f, glm::vec3(0.0f));
	}
	if (gTextRendererA) {
		gTextRendererA->RenderCenteredText(std::to_string(itemCount), -900.0f, 100.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	}

}

void FreeMainMenu()
{
	// Free resources specific to MainMenu state
	delete gTextRenderer;
	gTextRenderer = nullptr;

	delete gTextRendererA;
	gTextRendererA = nullptr;
}

void UnloadMainMenu()
{
	// Unload MainMenu state
}