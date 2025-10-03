#include "scenes/rain.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
#include "TextRenderer.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"
#include "sceneutils.h"
#include "managers/assetmanager.h"
#include "resources/sceneasset.h"

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "rain";

void LoadRain()
{
	// Load resources specific to Renfred state
	LOGI(Core, "Loaded");
	
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
}

void InitRain()
{
	// Initialize Rain state
	CrashoutsEngine::Serialization::LoadScenes(sceneName);

	LOGI(Core, "Rain Initialized");
}

void UpdateRain()
{
	// Update logic for Rain state
}

void RenderRain()
{
	// Render Rain state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	if (gTextRenderer) {
		gTextRenderer->RenderCenteredText("Hello", 0.0f, 0.0f, 1.0f, glm::vec3(1.0f));
	}

	Scene::RenderScene(*globalECS);
}

void FreeRain()
{
	// Free resources specific to Rain state
	CrashoutsEngine::Serialization::SaveScenes(sceneName);

	delete gTextRenderer;
	gTextRenderer = nullptr;
}

void UnloadRain()
{
	// Unload Rain state
}