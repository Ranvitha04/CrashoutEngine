#include "shaderasset.h"

#include "amirscene.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "sceneasset.h"

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "amir";

void LoadAmir()
{
	using namespace CrashoutsEngine::Assets;
	sceneName = "amir";
	// Load resources specific to amir state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	GenerateAssetManifest();

	AssetManager::GetInstance().LoadManifestAssets();

	AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();
}

void InitAmir()
{
	CrashoutsEngine::Serialization::LoadScenes(sceneName);

	LOGI(Core, "Amir Initialized");
}

void UpdateAmir()
{

}

void RenderAmir()
{
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FreeAmir()
{
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadAmir()
{

}