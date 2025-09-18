#include "scenes/rain.h"
#include "log.h"
#include "GLFW/glfw3.h"
void LoadRain()
{
	// Load resources specific to Rain state
}

void InitRain()
{
	// Initialize Rain state
	LOGI(Core, "Rain Initialized");
}

void UpdateRain()
{
	// Update logic for Rain state
}

void RenderRain()
{
	// Render Rain state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void FreeRain()
{
	// Free resources specific to Rain state
}

void UnloadRain()
{
	// Unload Rain state
}