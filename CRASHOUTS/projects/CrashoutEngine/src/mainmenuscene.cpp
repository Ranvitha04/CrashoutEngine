/*                                                   Includes
----------------------------------------------------------------------------- */
#include "mainmenuscene.h"

#include "sceneutils.h"
#include "textrenderer.h"
#include "log.h"
#include "rendererconnector.h"
#include "settings.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include "inputmanager.h"
#include "collisionmanager.h"
#include "physicsmanager.h"
#include <random>
#include <string>
#include <chrono> // Used to seed the random number generator
#include "frameratecontroller.h"
#include "keyframe.h"
#include "spritecomponent.h"   
#include "spriteAnimation.h"  
#include "scriptcomponent.h"
#include "glm/vec3.hpp"
#include "GLFW/glfw3.h"

#include "monomanager.h"
#include "collidercomponent.h"
#include "namecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"


float random_min_max(float const& min, float const& max) {
	// 1. Setup the Random Number Generator (RNG)

	// Use a high-quality entropy source (like std::random_device) to seed the engine
	// A better practice is often seeding once at the start of your program.
	static std::random_device rd;

	// Use a Mersenne Twister engine for high-quality pseudo-random numbers
	static std::mt19937 generator(rd());

	// 2. Define the Distribution

	// Create a distribution for floating-point numbers between 0.0 and 255.0 (inclusive)
	std::uniform_real_distribution<float> distribution(min, max);

	// 3. Generate and Return the Number

	// Pass the generator to the distribution to get the next random number
	return distribution(generator);
}
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;

static int itemCount = 0;

static CrashoutsEngine::Entity entity1;
static CrashoutsEngine::Entity entity2;
static CrashoutsEngine::Entity entity3;

static const char* WALK_SHEET_PATH =
"images/Character/Normal Mode/Chibi-Actions-SpriteSheet.png";

static const char* JUMP_SHEET_PATH =
"images/Character/New Artstyle/Switch-State-SpriteSheet.png";

static const int SHEET_COLS = 3;
static const int SHEET_ROWS = 3;

enum class PlayerSheet
{
	Walk,
	LightMode
};

static PlayerSheet gCurrentSheet = PlayerSheet::Walk;
static bool gZWasDown = false;   

static std::string s_entity1Script = "PlayerController";
static std::string s_entity3Script = "EnemyController";

void AddPhysicsEntity(CrashoutsEngine::Entity &entity, Component::RigidBodyComponent&& rigidBody, 
	Component::TransformComponent&& transform, Component::ShapeComponent &&shape) {

	entity = globalECS->CreateEntity();

	CEVec3 extents{ transform.size.x * 0.5f, transform.size.y * 0.5f, 0.f };

	globalECS->AddComponent<Component::NameComponent>(entity, Component::NameComponent{std::to_string(entity)});
	globalECS->AddComponent<Component::RigidBodyComponent>(entity, rigidBody);
	globalECS->AddComponent<Component::TransformComponent>(entity, transform);
	globalECS->AddComponent<Component::ShapeComponent>(entity, shape);
	globalECS->AddComponent<Component::ColliderComponent>(entity, Component::ColliderComponent{
		std::make_unique<Collision::Polygon>(Collision::Polygon::createOBB(transform.position - extents, transform.position + extents, 0.f, CEVec3{}))
	});
}

void LoadMainMenu()
{
	// Load resources specific to MainMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();

	if (entity1 && globalECS->HasComponent<CrashoutsEngine::ScriptComponent>(entity1)) {
		auto* sc = globalECS->GetComponent<CrashoutsEngine::ScriptComponent>(entity1);
		if (!sc->Scripts.empty())
			s_entity1Script = sc->Scripts[0].ClassName;
	}
	if (entity3 && globalECS->HasComponent<CrashoutsEngine::ScriptComponent>(entity3)) {
		auto* sc = globalECS->GetComponent<CrashoutsEngine::ScriptComponent>(entity3);
		if (!sc->Scripts.empty())
			s_entity3Script = sc->Scripts[0].ClassName;
	}

	globalECS->Reset();

	using namespace CrashoutsEngine::Assets;
	AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();

	gTextRenderer = std::make_unique<TextRenderer>();
	gTextRendererA = std::make_unique<TextRenderer>();
}

void InitMainMenu()
{
	AddPhysicsEntity(
		entity1,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{10.f, 10.f}, .position = CEVec3{0.f, 20.f, 0.f}},
		Component::ShapeComponent{.enableTexture=true, .textureName="Concept"}
	);
	if (!globalECS->HasComponent<CrashoutsEngine::ScriptComponent>(entity1)) {
		globalECS->AddComponent<CrashoutsEngine::ScriptComponent>(
			entity1,
			CrashoutsEngine::ScriptComponent(s_entity1Script, entity1)
		);
	}
	else {
		auto* sc = globalECS->GetComponent<CrashoutsEngine::ScriptComponent>(entity1);

		if (sc->Scripts.empty())
			sc->AddScript(s_entity1Script);
		else
			sc->Scripts[0].ClassName = s_entity1Script;

		sc->Destroy();
		sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
	}
	// After entity1 = AddPhysicsEntity(...)

	using namespace Component;

	
	CEVec2 worldSize{ 1.0f, 1.0f };


	SetSpriteSheet(
		entity1,
		WALK_SHEET_PATH,
		SHEET_COLS,   
		SHEET_ROWS,   
		worldSize
	);


	SpriteAnimComponent anim{};
	anim.startFrame = 0;    
	anim.frameCount = 1;      
	anim.frameTime = 0.10f;
	anim.frameIndex = 0;
	anim.timer = 0.0f;
	anim.flip = false;

	globalECS->AddComponent<SpriteAnimComponent>(entity1, anim);

	AddPhysicsEntity(
		entity2,
		Component::RigidBodyComponent{ CEVec3{}, 0.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{80.f, 10.f}, .position = CEVec3{0.f, -10.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle }
	);
	

	AddPhysicsEntity(
		entity3,
		Component::RigidBodyComponent{ CEVec3{}, 5.f, 0.f },
		Component::TransformComponent{ .size = CEVec2{10.f, 10.f}, .position = CEVec3{5.f, 20.f, 0.f} },
		Component::ShapeComponent{ .type = Component::ShapeType::Rectangle }
	);
	if (!globalECS->HasComponent<CrashoutsEngine::ScriptComponent>(entity3)) {
		globalECS->AddComponent<CrashoutsEngine::ScriptComponent>(
			entity3,
			CrashoutsEngine::ScriptComponent(s_entity3Script, entity3)
		);
	}
	else {
		auto* sc = globalECS->GetComponent<CrashoutsEngine::ScriptComponent>(entity3);

		if (sc->Scripts.empty())
			sc->AddScript(s_entity3Script);
		else
			sc->Scripts[0].ClassName = s_entity3Script;

		sc->Destroy();
		sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
	}
	// Initialize MainMenu state

	LOGI(Core, "MainMenu Initialized");
}

void CreateRandBox() {
	int entityId = globalECS->CreateEntity();
	globalECS->AddComponent<Component::NameComponent>(entityId, Component::NameComponent{ std::to_string(entityId) });

	Component::TransformComponent t;
	Component::ShapeComponent shape;
	// Set random pos rot scale
	float width = 100.f;
	float height = 50.f;
	t.position = CEVec3(random_min_max(-width, width), random_min_max(-height, height), 0.f);
	t.rotation = random_min_max(0.f, 360.f);
	t.size = CEVec2(random_min_max(.5f, 3.f), random_min_max(.5f, 3.f));

	shape.color = CEColor(random_min_max(0.f, 255.f), random_min_max(0.f, 255.f), random_min_max(0.f, 255.f));
	shape.type = Component::ShapeType::Rectangle;


	globalECS->AddComponent<Component::TransformComponent>(entityId, t);
	globalECS->AddComponent<Component::ShapeComponent>(entityId, shape);
	
	if (rand() % 2) {
		CEVec3 rb_init_velocity{ 0.f, 0.f, 0.f };
		float rb_mass = .5f;
		float rb_restitution = 0.f;
		globalECS->AddComponent<Component::RigidBodyComponent>(entityId, rb_init_velocity, rb_mass, rb_restitution);
	}
}
void UpdateMainMenu()
{
	if (!globalECS)
		return;

	float dt = static_cast<float>(FrameRateController::GetInstance().DT());
	KeyframeUnity::UpdateAll(*globalECS, dt);

	// --- Get components on entity1 (player in main menu) ---
	auto* rb = globalECS->GetComponent<Component::RigidBodyComponent>(entity1);
	auto* anim = globalECS->GetComponent<Component::SpriteAnimComponent>(entity1);

	bool isMovingHoriz = false;

	if (rb) {
		CEVec3 vel = rb->getVelocity();
		if (vel.x > 0.1f || vel.x < -0.1f)
			isMovingHoriz = true;
	}

	// --- Z toggle: swap between the two spritesheets ---
	bool zDown = InputManager::IsKeyPressed(Key::Z);

	if (zDown && !gZWasDown) {
		// Just pressed this frame
		if (gCurrentSheet == PlayerSheet::Walk) {
			gCurrentSheet = PlayerSheet::LightMode;

			Component::SetSpriteSheet(
				entity1,
				JUMP_SHEET_PATH,             // Switch-Style-SpriteSheet
				SHEET_COLS, SHEET_ROWS,
				CEVec2{ 1.0f, 1.0f }
			);
		}
		else {
			gCurrentSheet = PlayerSheet::Walk;

			Component::SetSpriteSheet(
				entity1,
				WALK_SHEET_PATH,             // Chibi-Actions-SpriteSheet
				SHEET_COLS, SHEET_ROWS,
				CEVec2{ 1.0f, 1.0f }
			);
		}
	}

	gZWasDown = zDown;

	// --- Simple animation region on the current sheet ---
	if (anim) {
		if (isMovingHoriz) {
			// walking: use frames 3–5 on the sheet
			anim->startFrame = 3;
			anim->frameCount = 3;
		}
		else {
			// idle: first frame
			anim->startFrame = 0;
			anim->frameCount = 1;
			anim->frameIndex = 0;
			anim->timer = 0.0f;
		}

		// Flip sprite based on input
		if (InputManager::IsKeyPressed(Key::A) || InputManager::IsKeyPressed(Key::Left)) {
			anim->flip = true;
		}
		else if (InputManager::IsKeyPressed(Key::D) || InputManager::IsKeyPressed(Key::Right)) {
			anim->flip = false;
		}

		Component::UpdateSpriteAnimationsECS();
	}

	// --- Keep your scale & rotate controls ---
	Component::TransformComponent* transform =
		globalECS->GetComponent<Component::TransformComponent>(entity1);

	if (!transform)
		return;

	// SCALE UP (E)
	if (InputManager::IsKeyPressed(Key::E)) {
		transform->size *= 0.9f;
	}

	// SCALE DOWN (Q)
	if (InputManager::IsKeyPressed(Key::Q)) {
		transform->size *= 1.1f;
	}

	// ROTATE (R)
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{
				globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type == Collision::ColliderType::OBB) {
				Collision::OBBRect& obbTemp2 =
					dynamic_cast<Collision::OBBRect&>(*(collider1->collider));
				obbTemp2.updateRotation(transform->rotation);
			}
		}
	}
}

/*
void UpdateMainMenu()
{
	if (!globalECS)
		return;

	
	float dt = static_cast<float>(FrameRateController::GetInstance().DT());
	KeyframeUnity::UpdateAll(*globalECS, dt);

	
	auto* rb = globalECS->GetComponent<Component::RigidBodyComponent>(entity1);
	auto* anim = globalECS->GetComponent<Component::SpriteAnimComponent>(entity1);

	bool isMovingHoriz = false;
	

	if (rb) {
		CEVec3 vel = rb->getVelocity();

		
		if (vel.x > 0.1f || vel.x < -0.1f)
			isMovingHoriz = true;

		
	}


	bool zDown = InputManager::IsKeyPressed(Key::Z);

	
	if (zDown && !gZWasDown) {
		
		if (gCurrentSheet == PlayerSheet::Walk) {
			gCurrentSheet = PlayerSheet::LightMode;

			
			Component::SetSpriteSheet(
				entity1,
				JUMP_SHEET_PATH,
				SHEET_COLS, SHEET_ROWS,
				CEVec2{ 1.0f, 1.0f }
			);
		}
		else {
			gCurrentSheet = PlayerSheet::Walk;

		
			Component::SetSpriteSheet(
				entity1,
				WALK_SHEET_PATH,
				SHEET_COLS, SHEET_ROWS,
				CEVec2{ 1.0f, 1.0f }
			);
		}
	}

	
	gZWasDown = zDown;


	if (anim) {
		if (isMovingHoriz) {
		
			anim->startFrame = 3;
			anim->frameCount = 3;
		}
		else {
		
			anim->startFrame = 0;
			anim->frameCount = 1;
			anim->frameIndex = 0;
			anim->timer = 0.0f;
		}

		
		if (InputManager::IsKeyPressed(Key::A) || InputManager::IsKeyPressed(Key::Left)) {
			anim->flip = true;  
		}
		else if (InputManager::IsKeyPressed(Key::D) || InputManager::IsKeyPressed(Key::Right)) {
			anim->flip = false;  
		}
	
		
		Component::UpdateSpriteAnimationsECS();
	}

	

	Component::TransformComponent* transform =
		globalECS->GetComponent<Component::TransformComponent>(entity1);

	if (!transform)
		return;

	// SCALE UP
	if (InputManager::IsKeyPressed(Key::E)) {
		transform->size *= 0.9f;
	}

	// SCALE DOWN
	if (InputManager::IsKeyPressed(Key::Q)) {
		transform->size *= 1.1f;
	}

	// ROTATE
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{
				globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type ==
				Collision::ColliderType::OBB) {
				Collision::OBBRect& obbTemp2 =
					dynamic_cast<Collision::OBBRect&>(*(collider1->collider));
				obbTemp2.updateRotation(transform->rotation);
			}
		}
	}
	//Component::RigidBodyComponent* rigidBody{ globalECS->GetComponent<Component::RigidBodyComponent>(entity1) };
	//Component::TransformComponent* transform{ globalECS->GetComponent<Component::TransformComponent>(entity1) };

	//Component::RigidBodyComponent* rigidBody2{ globalECS->GetComponent<Component::RigidBodyComponent>(entity3) };

	/*if (itemCount < 2500 && InputManager::IsKeyPressed(Key::Num1)) {
		int newCount = 0;
		while (newCount < 1) {
			CreateRandBox();
			newCount++;
			itemCount++;
		}
	}*/

	/*
	if (rigidBody) {
		if (InputManager::IsKeyPressed(Key::A)) rigidBody->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::D)) rigidBody->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::W)) rigidBody->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody->getMass());
	}
	*/
	/*
	if (rigidBody2) {
		if (InputManager::IsKeyPressed(Key::Left)) rigidBody2->addForce(CEVec3{ -100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Right)) rigidBody2->addForce(CEVec3{ 100.f, 0.f, 0.f });
		if (InputManager::IsKeyPressed(Key::Up)) rigidBody2->addForce(CEVec3{ 0.f, 300.f, 0.f }*rigidBody2->getMass());
	}
	*/
	/*
	// SCALE UP
	if (InputManager::IsKeyPressed(Key::S)) {
		transform->size *= 0.9f;
	}

	// SCALE DOWN 
	if (InputManager::IsKeyPressed(Key::Q)) {
		transform->size *= 1.1f;
	}
	
	// ROTATE
	if (InputManager::IsKeyPressed(Key::R)) {
		transform->rotation += 3.f;
		if (globalECS->HasComponent<Component::ColliderComponent>(entity1)) {
			Component::ColliderComponent* collider1{ globalECS->GetComponent<Component::ColliderComponent>(entity1) };
			if (collider1->collider->type == Collision::ColliderType::ColliderType::OBB) {
				Collision::OBBRect & obbTemp2 = dynamic_cast<Collision::OBBRect &>(*(collider1->collider));
				obbTemp2.updateRotation(transform->rotation);
			}
		}
	}*/


void RenderMainMenu()
{
	// Render MainMenu state
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	Component::RenderSpriteAnimationsECS();
	//Scene::RenderScene(*globalECS);
}

void FreeMainMenu()
{
	// Free resources specific to MainMenu state
	globalECS->DestroyEntity(entity1);
	globalECS->DestroyEntity(entity2);
	globalECS->DestroyEntity(entity3);
}

void UnloadMainMenu()
{
	// Unload MainMenu state
}