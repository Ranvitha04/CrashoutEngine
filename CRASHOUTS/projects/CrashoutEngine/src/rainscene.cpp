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