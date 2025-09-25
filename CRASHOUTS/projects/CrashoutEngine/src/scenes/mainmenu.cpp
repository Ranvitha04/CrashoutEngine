#include "scenes/mainmenu.h"
#include "managers/entitymanager.h"
#include "log.h"
#include "rendering.h"
#include "shapes.h"
#include "glapp.h"

#include "managers/entitymanager.h"

static CrashoutsEngine::ECS* globalECS;

void LoadMainMenu()
{
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();
	LOGI(Core, "Main Menu Loaded");
}

void InitMainMenu()
{
	LOGI(Core, "Main Menu Initialized");
}

void UpdateMainMenu()
{

}

void RenderMainMenu()
{
	// Render Main state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	using namespace CrashoutsEngine;

	const ECS::ComponentArray<Component::Shapes>& shapeArray = globalECS->GetComponentArray<Component::Shapes>();
	for (const Entity& id : shapeArray.indexToEntity) {
		// Get entity number
		Component::TransformComponent* t = globalECS->GetComponent<Component::TransformComponent>(id);
		// Can add more data to create rect like color etc
		if (globalECS->HasComponent<Shapes>(id))
			Component::Shapes::CreateRectangle(t->position, t->size.x, t->size.y, t->rotation);
	}
}

void FreeMainMenu()
{

}

void UnloadMainMenu()
{

}