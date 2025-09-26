#include "scenes/afiq.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"

// ---- FMOD headers ----
#include <fmod_studio.hpp>
#include <fmod_errors.h> 
#include <fmod.hpp>
#include <cstdio>
#include <thread>
#include <chrono>


static CrashoutsEngine::ECS* globalECS;

// ---- FMOD globals for this scene ----
static FMOD::Studio::System* gStudio = nullptr;
static FMOD::Studio::Bank* gBankStrings = nullptr;
static FMOD::Studio::Bank* gBankMaster = nullptr;
static FMOD::Studio::Bank* gBankSFX = nullptr;
static FMOD::Studio::EventInstance* gEvent = nullptr;

// Small helper to print FMOD errors
static bool ok(FMOD_RESULT r, const char* what) {
	if (r != FMOD_OK) {
		std::fprintf(stderr, "[FMOD] %s: %s\n", what, FMOD_ErrorString(r));
		return false;
	}
	return true;
}


void LoadAfiq()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();


    // --- Create + init FMOD Studio just for this scene ---
    if (!ok(FMOD::Studio::System::create(&gStudio), "System::create")) return;
    if (!ok(gStudio->initialize(
        64,                             // reasonable channel/instance pool
        FMOD_STUDIO_INIT_NORMAL,
        FMOD_INIT_NORMAL,
        nullptr),
        "System::initialize")) return;

    // --- Load banks ---
    // Adjust these paths to wherever your .bank files live.
    // Common pattern in repos: "assets/audio/banks/"
    ok(gStudio->loadBankFile("assets/audio/banks/Master.strings.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &gBankStrings), "load Master.strings.bank");
    ok(gStudio->loadBankFile("assets/audio/banks/Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &gBankMaster), "load Master.bank");
    ok(gStudio->loadBankFile("assets/audio/banks/sfx.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &gBankSFX), "load sfx.bank");

    // Optionally, you can preload sample data if you want zero-latency start:
    // if (gBankSFX) gBankSFX->loadSampleData();
}


void InitAfiq()
{
	LOGI(Core, "Afiq Initialized");
    // --- Create an event instance and start it ---
    // Change this to match your FMOD Studio Event path exactly (case-sensitive).
    // Example: "event:/UI/Beep" or "event:/sfexbeep"
    FMOD::Studio::EventDescription* desc = nullptr;
    if (gStudio && ok(gStudio->getEvent("event:/sfexbeep", &desc), "getEvent(event:/sfexbeep)")) {
        if (ok(desc->createInstance(&gEvent), "createInstance")) {
            ok(gEvent->start(), "start");
            // You can release your handle; FMOD keeps it alive while playing.
            // But keep gEvent if you want to manipulate it later (set volume/params/stop).
            // gEvent->release(); gEvent = nullptr; // uncomment if you don't need to control it
        }
    }
}

void UpdateAfiq()
{
    // Pump FMOD once per frame.
    if (gStudio) gStudio->update();

    // (Your scene’s update logic can go here)
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
			float rot{ 0.f };
			if (auto* tf = globalECS->GetComponent<Component::TransformComponent>(id)) {
				pos = tf->position;
				scale = tf->size;
				rot = tf->rotation;
			}
			switch (sc->type) {
			case Component::ShapeType::Square:    Component::Shapes::CreateSquare(pos, sc->size * scale.x, rot, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Rectangle: Component::Shapes::CreateRectangle(pos, sc->width * scale.x, sc->height * scale.y, rot, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Line:      Component::Shapes::CreateLine(sc->p1, sc->p2, sc->thickness, rot, sc->scale, sc->color * (1 / 255.f)); break;
			case Component::ShapeType::Triangle:  Component::Shapes::CreateTriangle(sc->t1, sc->t2, sc->t3, rot, sc->scale * scale.x, sc->color * (1 / 255.f)); break;
			}
		}
	}
}

void FreeAfiq()
{
    // Stop and release event instance if we kept a handle
    if (gEvent) {
        gEvent->stop(FMOD_STUDIO_STOP_IMMEDIATE);
        gEvent->release();
        gEvent = nullptr;
    }

    // You could also unload sample data explicitly here if you loaded it:
    // if (gBankSFX) gBankSFX->unloadSampleData();
}

void UnloadAfiq()
{
    // Unload banks (safe if null)
    if (gBankSFX) { gBankSFX->unload();     gBankSFX = nullptr; }
    if (gBankMaster) { gBankMaster->unload();  gBankMaster = nullptr; }
    if (gBankStrings) { gBankStrings->unload(); gBankStrings = nullptr; }

    // Tear down Studio system last
    if (gStudio) {
        gStudio->unloadAll();   // extra safety
        gStudio->release();
        gStudio = nullptr;
    }
}
