#include "scenes/ran.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"
#include "sceneutils.h"
#include "managers/assetmanager.h"
#include "resources/sceneasset.h"

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "renfred";

void LoadRan()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
}

void InitRan()
{
	LOGI(Core, "Ran Initialized");
}

void UpdateRan()
{

}

void RenderRan()
{
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Scene::RenderScene(*globalECS);
}

void FreeRan()
{
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadRan()
{
}