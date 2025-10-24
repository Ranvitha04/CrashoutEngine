#include "tristanscene.h"
#include "log.h"
#include "glapp.h"
#include "CEVec3.h"
#include "shapes.h"
#include "./entitymanager.h"
#include "./collisionmanager.h"
#include "./physicsmanager.h"
#include "./inputmanager.h"
#include "assetmanager.h"
#include "sceneasset.h"
#include "sceneutils.h"
#include <memory>


static CrashoutsEngine::ECS* globalECS; /*STATIC FOR INTERNAL LINKAGE*/
CrashoutsEngine::Entity entity1;
CrashoutsEngine::Entity entity2;
CrashoutsEngine::Entity entity3;
CrashoutsEngine::Entity entity4;



void LoadTristan() {
	LOGI(Core, "Tristan Initialized");

	using namespace CrashoutsEngine::Assets;
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();




	GenerateAssetManifest();

	AssetManager::GetInstance().LoadManifestAssets();


}

void InitTristan() {
	CrashoutsEngine::Serialization::LoadScenes("tristan");
}

void UpdateTristan()
{
	

}

void RenderTristan() {
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	using namespace CrashoutsEngine;

	Scene::RenderScene(*globalECS);

}

void FreeTristan() {

}

void UnloadTristan()
{
	
}