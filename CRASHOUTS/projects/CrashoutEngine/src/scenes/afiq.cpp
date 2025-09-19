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
}

void FreeAfiq()
{
	
}

void UnloadAfiq()
{
	
}
