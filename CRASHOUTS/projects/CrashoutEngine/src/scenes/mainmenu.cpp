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

	const ECS::ComponentArray<Component::ShapeComponent>& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
	//std::cout << shapeArray.dense.size() << std::endl;
	for (const Entity& id : shapeArray.indexToEntity) {
		// Get entity number
		Component::TransformComponent* t = globalECS->GetComponent<Component::TransformComponent>(id);
		Component::ShapeComponent* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
		// Can add more data to create rect like color etc
		if (globalECS->HasComponent<ShapeComponent>(id)) {

			CEVec3 pos{ 0,0,0 };
			CEVec2 scale{ 1.f, 1.f };
			if (auto* tf = globalECS->GetComponent<Component::TransformComponent>(id)) {
				pos = tf->position;
				scale = tf->size;
			}
			switch (sc->type) {
			case Component::ShapeType::Square:    Component::Shapes::CreateSquare(pos, sc->size * scale.x, sc->rotation, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Rectangle: Component::Shapes::CreateRectangle(pos, sc->width * scale.x, sc->height * scale.y, sc->rotation, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Line:      Component::Shapes::CreateLine(sc->p1, sc->p2, sc->thickness, sc->rotation, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Triangle:  Component::Shapes::CreateTriangle(sc->t1, sc->t2, sc->t3, sc->rotation, sc->scale * scale.x, sc->color * (1 / 255.f)); break;
			}
		}
	}
}

void FreeMainMenu()
{

}

void UnloadMainMenu()
{

}