/*!
@file        renfred.cpp
@author      lee.r@digipen.edu
@date        09/18/2025
@brief       Implements the 

This file implements
*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "scenes/renfred.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"

/*                                                   Global Variables
----------------------------------------------------------------------------- */

CrashoutsEngine::ECS* globalECS;

void LoadRenfred()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
}

void InitRenfred()
{
	// Initialize Renfred state
	LOGI(Core, "Renfred Initialized");
}

void UpdateRenfred()
{
	// Update logic for Renfred state
}

void RenderRenfred()
{
	// Render Renfred state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	using namespace CrashoutsEngine;

	const ECS::ComponentArray<Component::Shapes>& shapeArray = globalECS->GetComponentArray<Component::Shapes>();
	for (const Entity& id : shapeArray.indexToEntity) {
		// Get entity number
		Component::TransformComponent* t = globalECS->GetComponent<Component::TransformComponent>(id);
		// Can add more data to create rect like color etc
		if(globalECS->HasComponent<Shapes>(id))
			Component::Shapes::CreateRectangle(t->position, t->size.x, t->size.y, t->rotation);
	}

}

void FreeRenfred()
{
	// Free resources specific to Renfred state
}

void UnloadRenfred()
{
	// Unload Renfred state
}