#include "scenes/amir.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"

static CrashoutsEngine::ECS* globalECS;

void LoadAmir()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();
}

void InitAmir()
{
	LOGI(Core, "Amir Initialized");
}

void UpdateAmir()
{

}

void RenderAmir()
{
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void FreeAmir()
{

}

void UnloadAmir()
{

}