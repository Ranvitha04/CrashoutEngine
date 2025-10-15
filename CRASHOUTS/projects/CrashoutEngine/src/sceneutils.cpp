/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the

This file implements
*/

#include "sceneutils.h"
#include "textureasset.h"
#include "assetmanager.h"

namespace Scene {
	void RenderScene(CrashoutsEngine::ECS& globalECS) {
		using namespace CrashoutsEngine;

		const ECS::ComponentArray<Component::ShapeComponent>& shapeArray = globalECS.GetComponentArray<Component::ShapeComponent>();
		//std::cout << shapeArray.dense.size() << std::endl;
		for (const Entity& id : shapeArray.indexToEntity) {
			// Get entity number
			//Component::TransformComponent* t = globalECS.GetComponent<Component::TransformComponent>(id);
			Component::ShapeComponent* sc = globalECS.GetComponent<Component::ShapeComponent>(id);
			// Can add more data to create rect like color etc
			if (globalECS.HasComponent<ShapeComponent>(id)) {

				CEVec3 pos{ 0,0,0 };
				CEVec2 scale{ 1.f, 1.f };
				float rot{ 0.f };
				if (auto* tf = globalECS.GetComponent<Component::TransformComponent>(id)) {
					pos = tf->position;
					scale = tf->size;
					rot = tf->rotation;
				}
				CEVec2 finalScale{ sc->width * scale.x, sc->height * scale.y };

				if (sc->enableTexture) {
					using namespace Assets;
					TextureAsset* tex = Assets::AssetManager::GetInstance().Get<TextureAsset>(sc->textureName);
					if (tex) {
						tex->SetTextureMode(
							static_cast<TextureMode>(sc->wrapModeU),
							static_cast<TextureMode>(sc->wrapModeV)
						);
					}
				}

				switch (sc->type) {
				case Component::ShapeType::Square:    Component::Shapes::CreateSquare(pos, sc->size, rot, 
					(scale.x > scale.y) ? scale.x : scale.y, sc->color * (1 / 255.f)); break;
				// ==========================================RECTANGLE===================================================
				case Component::ShapeType::Rectangle:
					if (sc->enableTexture) {
						Component::Shapes::CreateTexturedRectangle(sc->textureName,
							pos, sc->width * scale.x, sc->height * scale.y, rot, 1.0f, sc->color * (1 / 255.f), sc->uvSize); 
						break;
					}
					else Component::Shapes::CreateRectangle(pos, sc->width * scale.x, sc->height * scale.y, rot, 1.0f, 
						sc->color * (1 / 255.f)); break;
				// ============================================LINE======================================================
				case Component::ShapeType::Line:
					Component::Shapes::CreateLine(sc->p1 + pos, sc->p2 + pos, sc->thickness, rot, (scale.x > scale.y) ? scale.x : scale.y,
						sc->color * (1 / 255.f)); break;
				// ==========================================TRIANGLE====================================================
				case Component::ShapeType::Triangle:
					if (sc->enableTexture) {
						Component::Shapes::CreateTexturedTriangle(sc->textureName,
							sc->t1 + pos, sc->t2 + pos, sc->t3 + pos, rot, (scale.x > scale.y) ? scale.x : scale.y, 
							sc->color * (1 / 255.f), sc->uvSize); break;
					}
					else Component::Shapes::CreateTriangle(sc->t1 + pos, sc->t2 + pos, sc->t3 + pos, rot, 
						(scale.x > scale.y) ? scale.x : scale.y, sc->color * (1 / 255.f)); break;
				}
			}
		}
	}
}