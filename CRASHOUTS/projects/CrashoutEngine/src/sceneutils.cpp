/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the

This file implements
*/

#include "rendering.h"
#include "textrenderer.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "settings.h"
#include "rendererconnector.h"
#include "sceneutils.h"

namespace Scene {
    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        using Component::Rendering;

        Rendering& renderer = Rendering::Get();
        if (!renderer.shdr_pgm) return;

        renderer.shdr_pgm->Use();

        renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

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
            Component::TransformComponent* tf = globalECS.GetComponent<Component::TransformComponent>(id);
            Component::ShapeComponent* sc = globalECS.GetComponent<Component::ShapeComponent>(id);
            // Can add more data to create rect like color etc
            if (globalECS.HasComponent<ShapeComponent>(id)) {

                CEVec3 pos{ 0,0,0 };
                CEVec2 scale{ 1.f, 1.f };
                float rot{ 0.f };
                if (tf) {
                    pos = tf->position;
                    scale = tf->size;
                    rot = tf->rotation;
                }

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
                // ==========================================RECTANGLE===================================================
                case Component::ShapeType::Rectangle:
                    if (useTexture) {
                        Component::Shapes::CreateTexturedRectangle(tf->size,
                            sc->textureName, pos, rot, sc->color * (1 / 255.f), sc->uvSize);
                        break;
                    } else {
                        Component::Shapes::CreateRectangle(tf->size,
                            pos, rot, sc->color * (1 / 255.f)
                        );
                        break;
                    }
                // ============================================LINE======================================================
                case Component::ShapeType::Line:
                    Component::Shapes::CreateLine(tf->size, sc->p1 + pos, sc->p2 + pos, sc->thickness, rot, sc->color * (1 / 255.f)); break;
                // ==========================================TRIANGLE====================================================
                case Component::ShapeType::Triangle:
                    if (useTexture) {
                        Component::Shapes::CreateTexturedTriangle(tf->size, sc->textureName,
                            pos, rot, sc->color * (1 / 255.f), sc->uvSize
                        );
                        break;
                    } else {
                        Component::Shapes::CreateTriangle(tf->size, pos,
                            rot, sc->color * (1 / 255.f)
                        );
                        break;
                    }
                // ==========================================CIRCLE======================================================
                case Component::ShapeType::Circle:
                    Component::Shapes::CreateCircle(tf->size, tf->position, 32, tf->rotation, sc->color * (1 / 255.f)); break;
                }
            }
        }
    
        // Perform instancing
        int instancedCount = static_cast<int>(instancedPos.size());
        if (instancedCount > 0) {
           
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