#include "shaderasset.h"

#include "rainscene.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "sceneasset.h"

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "rain";

void LoadRain()
{
	using namespace CrashoutsEngine::Assets;
	sceneName = "rain";
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	GenerateAssetManifest();

	AssetManager::GetInstance().LoadManifestAssets();

	AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();
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
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
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