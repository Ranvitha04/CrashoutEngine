/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the

This file implements
*/

#include "textrenderer.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "settings.h"
#include "rendererconnector.h"
#include "sceneutils.h"

namespace Scene {
    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;

        // Collect all non-textured squares here
        std::vector<CEVec3> instancedPos;
        std::vector<CEVec3> instancedColor;
        std::vector<float> instancedRot;
        std::vector<float> instancedScales;

        // Pre-allocate memory
        const ECS::ComponentArray<Component::ShapeComponent>& shapeArray = globalECS.GetComponentArray<Component::ShapeComponent>();
        //std::cout << shapeArray.dense.size() << std::endl;
        // Reserve based on the total number of shapes as a rough estimate
        size_t shapeSize{ shapeArray.dense.size() };
        instancedPos.reserve(shapeSize);
        instancedColor.reserve(shapeSize);
        instancedRot.reserve(shapeSize);
        instancedScales.reserve(shapeSize);
        
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

                // Strictly respect the ECS toggle
                bool useTexture = sc->enableTexture;

                if (useTexture) {
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
                case Component::ShapeType::Square:
                    if (useTexture) {
                        Component::Shapes::CreateTexturedSquare(
                            sc->textureName, pos, sc->size, rot,
                            (scale.x > scale.y) ? scale.x : scale.y,
                            sc->color * (1 / 255.f), sc->uvSize
                        );
                        break;
                    } else {

                        // Add data to our vectors
                        instancedPos.push_back(pos);
                        instancedColor.push_back(sc->color * (1 / 255.f));
                        instancedRot.push_back(rot);
                        float uniformTransformScale = (finalScale.x > finalScale.y) ? finalScale.x : finalScale.y;
                        instancedScales.push_back(uniformTransformScale);

                        //Component::Shapes::CreateSquare(
                        //    pos, sc->size, rot,
                        //    (scale.x > scale.y) ? scale.x : scale.y,
                        //    sc->color * (1 / 255.f)
                        //);
                        break;
                    }
                // ==========================================RECTANGLE===================================================
                case Component::ShapeType::Rectangle:
                    if (useTexture) {
                        Component::Shapes::CreateTexturedRectangle(
                            sc->textureName, pos, sc->width * scale.x, sc->height * scale.y,
                            rot, 1.0f, sc->color * (1 / 255.f), sc->uvSize
                        );
                        break;
                    } else {
                        Component::Shapes::CreateRectangle(
                            pos, sc->width * scale.x, sc->height * scale.y,
                            rot, 1.0f, sc->color * (1 / 255.f)
                        );
                        break;
                    }
                // ============================================LINE======================================================
                case Component::ShapeType::Line:
                    Component::Shapes::CreateLine(sc->p1 + pos, sc->p2 + pos, sc->thickness, rot, (scale.x > scale.y) ? scale.x : scale.y,
                        sc->color * (1 / 255.f)); break;
                // ==========================================TRIANGLE====================================================
                case Component::ShapeType::Triangle:
                    if (useTexture) {
                        Component::Shapes::CreateTexturedTriangle(
                            sc->textureName,
                            sc->t1 + pos, sc->t2 + pos, sc->t3 + pos,
                            rot, (scale.x > scale.y) ? scale.x : scale.y,
                            sc->color * (1 / 255.f), sc->uvSize
                        );
                        break;
                    } else {
                        Component::Shapes::CreateTriangle(
                            sc->t1 + pos, sc->t2 + pos, sc->t3 + pos,
                            rot, (scale.x > scale.y) ? scale.x : scale.y,
                            sc->color * (1 / 255.f)
                        );
                        break;
                    }
                }
            }
        }
    
        // Test instancing squares
        int instancedCount = static_cast<int>(instancedPos.size());
        if (instancedCount > 0) {
            Component::Shapes::CreateInstancedSquares(
                instancedPos.data(),
                instancedColor.data(),
                instancedRot.data(),
                instancedScales.data(),
                instancedCount,
                1.0f // This baseSize matches the 1.0f used in your initialization
            );
        }

		// For Drawing Text Components
		const ECS::ComponentArray<Component::TextComponent>& textArray = globalECS.GetComponentArray<Component::TextComponent>();
        if (!gTextRenderer) {
            gTextRenderer = std::make_unique<TextRenderer>();
        }

        for (const Entity& id : textArray.indexToEntity) {
            Component::TextComponent* tc = globalECS.GetComponent<Component::TextComponent>(id);
            if (globalECS.HasComponent<Component::TransformComponent>(id)) {
                Component::TransformComponent* t = globalECS.GetComponent<Component::TransformComponent>(id);
                CEVec2 newOffset = CEVec2{ t->position.x + tc->offset.x, t->position.y + tc->offset.y};

                std::string font = tc->font;
                if (font == "") font = "Exo2-Regular";

                if(tc->centered)
                    gTextRenderer->RenderCenteredText(tc->text, newOffset.x, newOffset.y, tc->scale, tc->color, font);
                else
					gTextRenderer->RenderText(tc->text, newOffset.x, newOffset.y, tc->scale, tc->color, font);
            }
        }
    }
}