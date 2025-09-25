#include "scenes/afiq.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"

static CrashoutsEngine::ECS* globalECS;

void LoadAfiq()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();
}


void InitAfiq()
{
	LOGI(Core, "Afiq Initialized");
}

void UpdateAfiq()
{
	
}

void RenderAfiq()
{
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

void FreeAfiq()
{
	
}

void UnloadAfiq()
{
	
}
