#include "scenes/tristan.h"
#include "log.h"
#include "glapp.h"
#include "math/CEVec3.h"
#include "shapes.h"
#include "./managers/entitymanager.h"
#include "./managers/collisionmanager.h"
#include "./managers/physicsmanager.h"
#include "./managers/inputmanager.h"
#include <memory>


static CrashoutsEngine::ECS* globalECS; /*STATIC FOR INTERNAL LINKAGE*/
CrashoutsEngine::Entity entity1;
CrashoutsEngine::Entity entity2;
CrashoutsEngine::Entity entity3;
CrashoutsEngine::Entity entity4;



void LoadTristan() {
	LOGI(Core, "Tristan Initialized");
	globalECS = CrashoutsEngine::ECS::getInstance();
	entity1 = globalECS->CreateEntity();
	entity2 = globalECS->CreateEntity();
	entity3 = globalECS->CreateEntity();
	entity4 = globalECS->CreateEntity();

	globalECS->AddComponent<Component::NameComponent>(entity1, Component::NameComponent{ "SCENE_EX" });
	globalECS->AddComponent<Component::TransformComponent>(entity1, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{0.f, 20.f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity1, Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity1, Component::ColliderComponent{ std::make_unique<Collision::Sphere>(Collision::Sphere{
		10.0, CEVec3{}
	}) });
	globalECS->AddComponent<Component::ShapeComponent>(entity1, Component::ShapeComponent{.type=Component::ShapeType::Rectangle, .width=20.f, .height=20.f});

	globalECS->AddComponent<Component::NameComponent>(entity2, Component::NameComponent{ "SCENE_EX2" });
	globalECS->AddComponent<Component::TransformComponent>(entity2, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{30.f, 20.f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity2, Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity2, Component::ColliderComponent{ std::make_unique<Collision::Sphere>(Collision::Sphere{
		10.0, CEVec3{}
	}) });
	globalECS->AddComponent<Component::ShapeComponent>(entity2, Component::ShapeComponent{ .type = Component::ShapeType::Rectangle, .width = 20.f, .height = 20.f });

	/*globalECS->AddComponent<Component::NameComponent>(entity4, Component::NameComponent{ "SCENE_EX2" });
	globalECS->AddComponent<Component::TransformComponent>(entity4, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{40.f, 20.f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity4, Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity4, Component::ColliderComponent{ std::make_unique<Collision::Sphere>(Collision::Sphere{
		10.0, CEVec3{}
	}) });
	globalECS->AddComponent<Component::ShapeComponent>(entity4, Component::ShapeComponent{ .type = Component::ShapeType::Rectangle, .width = 20.f, .height = 20.f });*/


	globalECS->AddComponent<Component::NameComponent>(entity3, Component::NameComponent{ "SCENE_EX3" });
	globalECS->AddComponent<Component::TransformComponent>(entity3, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{0.0f, 0.0f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity3, Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity3, Component::ColliderComponent{ std::make_unique<Collision::AABBRect>(Collision::AABBRect{
		CEVec3{-50.f, -5.f, 0.f},
		CEVec3{50.f, 5.f, 0.f}
	}) });
	globalECS->AddComponent<Component::ShapeComponent>(entity3, Component::ShapeComponent{ .type = Component::ShapeType::Rectangle, .width = 100.f, .height = 10.f });

}

void InitTristan() {
	
}

void UpdateTristan()
{
	Component::RigidBodyComponent *rigidBody { globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };

	if (InputManager::IsKeyPressed(Key::A)) rigidBody->addForce(CEVec3{ -100.f, 0.f, 0.f });
	if (InputManager::IsKeyPressed(Key::D)) rigidBody->addForce(CEVec3{ 100.f, 0.f, 0.f });
	if (InputManager::IsKeyPressed(Key::W)) rigidBody->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody->getMass());

}

void RenderTristan() {
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	using namespace CrashoutsEngine;

	//const ECS::ComponentArray<Component::ShapeComponent>& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
	//std::cout << shapeArray.dense.size() << std::endl;
	const ECS::ComponentArray<Component::ShapeComponent>& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
	//std::cout << shapeArray.dense.size() << std::endl;
	for (const Entity& id : shapeArray.indexToEntity) {

		if (globalECS->HasComponent<TransformComponent>(id)) {
			Component::TransformComponent* t = globalECS->GetComponent<Component::TransformComponent>(id);
			Component::ShapeComponent* sc = globalECS->GetComponent<Component::ShapeComponent>(id);

			if (globalECS->HasComponent<ColliderComponent>(id)) {


				CEVec2 scale{ 1.f, 1.f };
				Component::ColliderComponent* cc = globalECS->GetComponent<Component::ColliderComponent>(id);
				switch (sc->type) {
				case Component::ShapeType::Square:    Component::Shapes::CreateSquare(t->position, sc->size, t->rotation, 1.f, sc->color * (1 / 255.f)); break;
				case Component::ShapeType::Rectangle: Component::Shapes::CreateRectangle(t->position, sc->width, sc->height * 1.f, t->rotation, 1.f, sc->color * (1 / 255.f)); break;
				case Component::ShapeType::Line:      Component::Shapes::CreateLine(sc->p1, sc->p2, sc->thickness, t->rotation, (scale.x > scale.y) ? scale.x : scale.y, sc->color * (1 / 255.f)); break;
				case Component::ShapeType::Triangle:  Component::Shapes::CreateTriangle(sc->t1, sc->t2, sc->t3, t->rotation, (scale.x > scale.y) ? scale.x : scale.y, sc->color * (1 / 255.f)); break;
				}
			}

		}
	}

}

void FreeTristan() {

}

void UnloadTristan()
{
	globalECS->DestroyEntity(entity1);
	globalECS->DestroyEntity(entity2);
	globalECS->DestroyEntity(entity3);
}