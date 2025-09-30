/*                                                   Includes
----------------------------------------------------------------------------- */
#include "scenes/mainmenu.h"

#include "sceneutils.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
//#include "GLFW/glfw3.h"
#include "glm/vec3.hpp"
#include "managers/entitymanager.h"
#include "managers/assetmanager.h"
#include "textrenderer.h"


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
	if (gTextRenderer) {
		gTextRenderer->renderCenteredText("Hello", -900.0f, 0.0f, 1.0f, glm::vec3(1.0f));
	}
	if (gTextRendererA) {
		gTextRendererA->renderCenteredText("Bye", -900.0f, 100.0f, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	Scene::RenderScene(*globalECS);
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