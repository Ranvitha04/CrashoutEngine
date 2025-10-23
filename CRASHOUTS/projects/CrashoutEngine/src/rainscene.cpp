#include "rainscene.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
#include "TextRenderer.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "sceneutils.h"
#include "assetmanager.h"
#include "sceneasset.h"
#include "shaderasset.h"

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "rain";

void LoadRain()
{
	// Load resources specific to Renfred state
	LOGI(Core, "Loaded");
	
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	using namespace CrashoutsEngine::Assets;

	GenerateAssetManifest();

	AssetManager::GetInstance().LoadManifestAssets();

	AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();

	gTextRenderer = std::make_unique<TextRenderer>();
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (gTextRenderer) {
		gTextRenderer->RenderCenteredText("Hello", 0.0f, 0.0f, 1.0f, CEVec3{ 1.0f, 1.f, 1.f });
	}

	Scene::RenderScene(*globalECS);
}

void FreeRain()
{
	// Free resources specific to Rain state
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadRain()
{
	// Unload Rain state
}