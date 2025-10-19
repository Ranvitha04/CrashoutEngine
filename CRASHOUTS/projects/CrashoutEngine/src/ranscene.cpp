#include "ranscene.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "sceneutils.h"
#include "assetmanager.h"
#include "sceneasset.h"

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

void HandleSpriteKeyInput(int key, int action)
{
    using Component::Sprite;

    // Respond to presses and repeats; releases don’t need state changes here
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_LEFT:
            case GLFW_KEY_A:
                Sprite::m_sprite.setFlipHorizontal(true);
                break;
            case GLFW_KEY_RIGHT:
            case GLFW_KEY_D:
                Sprite::m_sprite.setFlipHorizontal(false);
                break;
            default:
                break;
        }
    }
}