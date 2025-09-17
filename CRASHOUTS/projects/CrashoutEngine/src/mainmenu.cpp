#include "mainmenu.h"
#include "managers/entitymanager.h"
#include "log.h"
#include "rendering.h"
#include "shapes.h"
#include "glapp.h"

void LoadMainMenu()
{
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
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	// ---- choose which shape to draw by uncommenting ----
	CEVec3 pos(0.0f, 0.0f, 0.0f);
	float rotation = 35.0f;
	float scale = 1.0f;

   // Testing for drawing Square
    Component::Shapes::CreateSquare(pos, 0.5f, rotation, scale, CEVec3(.0f, 1.0f, 0.0f));
}

void FreeMainMenu()
{

}

void UnloadMainMenu()
{

}