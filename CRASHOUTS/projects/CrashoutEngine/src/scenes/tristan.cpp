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


void LoadTristan() {
	LOGI(Core, "Tristan Initialized");
	globalECS = CrashoutsEngine::ECS::getInstance();
	entity1 = globalECS->CreateEntity();
	entity2 = globalECS->CreateEntity();
	entity3 = globalECS->CreateEntity();

	globalECS->AddComponent<Component::NameComponent>(entity1, Component::NameComponent{ "SCENE_EX" });
	globalECS->AddComponent<Component::TransformComponent>(entity1, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{0.f, 20.f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity1, Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity1, Component::ColliderComponent{ std::make_unique<Collision::Sphere>(Collision::Sphere{
		2.0, CEVec3{}
	}) });

	globalECS->AddComponent<Component::NameComponent>(entity2, Component::NameComponent{ "SCENE_EX2" });
	globalECS->AddComponent<Component::TransformComponent>(entity2, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{40.f, 20.f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity2, Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity2, Component::ColliderComponent{ std::make_unique<Collision::AABBRect>(Collision::AABBRect{
		CEVec3{0.f, 0.f, 0.f},
		CEVec3{6.f, 6.f, 0.f}
	}) });


	globalECS->AddComponent<Component::NameComponent>(entity3, Component::NameComponent{ "SCENE_EX3" });
	globalECS->AddComponent<Component::TransformComponent>(entity3, Component::TransformComponent{ 0.f, CEVec2{}, CEVec3{0.5f, 0.0f, 0.f} });
	globalECS->AddComponent<Component::RigidBodyComponent>(entity3, Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f });
	globalECS->AddComponent<Component::ColliderComponent>(entity3, Component::ColliderComponent{ std::make_unique<Collision::AABBRect>(Collision::AABBRect{
		CEVec3{0.f, 0.f, 0.f},
		CEVec3{50.f, 10.f, 0.f}
	}) });


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

	const CrashoutsEngine::ECS::ComponentArray<Component::NameComponent>& shapeArray = globalECS->GetComponentArray<Component::NameComponent>();
	for (const CrashoutsEngine::Entity & id : shapeArray.indexToEntity) {


		if (globalECS->HasComponent<Component::RigidBodyComponent>(id) && globalECS->HasComponent<Component::ColliderComponent>(id) && globalECS->HasComponent<Component::NameComponent>(id)) {
			Component::TransformComponent* t = globalECS->GetComponent<Component::TransformComponent>(id);
			Component::ColliderComponent const& collider{ *globalECS->GetComponent<Component::ColliderComponent>(id) };
			switch (collider.collider->type) {
				case Collision::SPHERE: {
					Collision::Sphere const& sphere = dynamic_cast<Collision::Sphere const&>(*(collider.collider));
					Component::Shapes::CreateRectangle(t->position, sphere.radius*2.f, sphere.radius*2.f, t->rotation);
				}
				break;

				case Collision::AABB: {
					Collision::AABBRect const& aabb = dynamic_cast<Collision::AABBRect const&>(*(collider.collider));
					Component::Shapes::CreateRectangle(t->position, aabb.max.x*2.f, aabb.max.y*2.f, t->rotation);
				}
				break;
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