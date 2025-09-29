/*                                                   Includes
----------------------------------------------------------------------------- */
#include "scenes/mainmenu.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"
#include "managers/assetmanager.h"


/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;

void LoadMainMenu()
{
	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
}

void InitMainMenu()
{
	// Initialize MainMenu state
	LOGI(Core, "MainMenu Initialized");
}

void UpdateMainMenu()
{
	// Update logic for MainMenu state
}

void RenderMainMenu()
{
	// Render MainMenu state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	Scene::RenderScene(*globalECS);
}

void FreeMainMenu()
{
	// Free resources specific to MainMenu state
}

void UnloadMainMenu()
{
	// Unload MainMenu state
}