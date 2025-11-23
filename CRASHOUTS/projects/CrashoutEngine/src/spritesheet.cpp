/*!
@file       spritesheet.cpp
@author     b.mohamadafiq@digipen.edu
@date       19/11/2025

*/


#include "spritesheet.h"
#include "spriteAnimation.h"
#include "rendering.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "entitymanager.h"
#include "spritecomponent.h"
#include "transformcomponent.h"
#include "frameratecontroller.h"
#include "CEVec2.h"
#include "CEVec3.h"
#include <filesystem>

#include <imgui.h>
#include "stb_image.h"          
#define STB_IMAGE_WRITE_IMPLEMENTATION   
#include "stb_image_write.h"   
#include <vector>
#include <cstring>
#include <algorithm>

using namespace CrashoutsEngine;   
using namespace Component;         
namespace fs = std::filesystem;
namespace UI
{
    
    struct SpriteClipUI
    {
        char  name[32];
        int   startFrame;
        int   frameCount;
        float fps;
        bool  loop;
    };

 
    static char  gSpritePath[256] = "assets/images/ducky_2_spritesheet.png";
    static int   gTileW = 32;   
    static int   gTileH = 32;  


    static int   gRows = 4;
    static int   gCols = 6;

    static int   gSelectedFrame = 0;
    static float gZoom = 2.0f;

    static bool  gClipsInit = false;
    static std::vector<SpriteClipUI> gClips;

  
    static void EnsureDefaultClips()
    {
        if (gClipsInit)
            return;

        gClipsInit = true;
        gClips.clear();

  
        SpriteClipUI idle{};
        std::strcpy(idle.name, "idle_normal");
        idle.startFrame = 0;
        idle.frameCount = 2;
        idle.fps = 4.0f;
        idle.loop = true;
        gClips.push_back(idle);

        SpriteClipUI walk{};
        std::strcpy(walk.name, "walk_normal");
        walk.startFrame = 6;
        walk.frameCount = 6;
        walk.fps = 8.0f;
        walk.loop = true;
        gClips.push_back(walk);
    }

    static void ExportCurrentSpritesheetToPNGs()
    {

        int width = 0, height = 0, channels = 0;
        const int desiredChannels = 4;

        unsigned char* pixels = stbi_load(
            gSpritePath, &width, &height, &channels, desiredChannels
        );
        if (!pixels) {
            std::printf("SpriteSheet export failed: cannot load '%s'\n", gSpritePath);
            return;
        }

        if (gTileW <= 0 || gTileH <= 0) {
            std::printf("SpriteSheet export failed: tile W/H must be > 0\n");
            stbi_image_free(pixels);
            return;
        }

        const int tileW = gTileW;
        const int tileH = gTileH;

        if (width < tileW || height < tileH) {
            std::printf("SpriteSheet export failed: image smaller than tile (%d x %d vs %d x %d)\n",
                width, height, tileW, tileH);
            stbi_image_free(pixels);
            return;
        }

        // How many tiles fit?
        int cols = width / tileW;
        int rows = height / tileH;

        if (cols <= 0 || rows <= 0) {
            std::printf("SpriteSheet export failed: invalid rows/cols from tile size\n");
            stbi_image_free(pixels);
            return;
        }

        // Optional: warn if extra pixels are cropped off
        if (width % tileW != 0 || height % tileH != 0) {
            std::printf("Warning: sheet not evenly divisible by tile size; extra pixels ignored.\n");
        }

        // Store for preview / selection
        gCols = cols;
        gRows = rows;
        if (tileW <= 0 || tileH <= 0) {
            std::printf("SpriteSheet export failed: invalid tile size (%d x %d)\n", tileW, tileH);
            stbi_image_free(pixels);
            return;
        }


        std::string path(gSpritePath);
        std::string dir, base, stem;


        std::size_t slash = path.find_last_of("/\\");
        if (slash == std::string::npos) {
            dir = "";
            base = path;
        }
        else {
            dir = path.substr(0, slash + 1);
            base = path.substr(slash + 1);
        }

        std::size_t dot = base.find_last_of('.');
        if (dot == std::string::npos)
            stem = base;
        else
            stem = base.substr(0, dot);


        fs::path outDir = "././projects/Descent/assets/spritesheet/";
        fs::create_directories(outDir);           
        std::string prefix = (outDir / stem).generic_string() + "_";




        const int tileStride = tileW * desiredChannels;
        const std::size_t tileSize = (std::size_t)tileW * tileH * desiredChannels;
        std::vector<unsigned char> tile(tileSize);

        int index = 0;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const int x0 = c * tileW;
                const int y0 = r * tileH;
           
                for (int y = 0; y < tileH; ++y) {
                    const unsigned char* srcRow =
                        pixels + ((y0 + y) * width + x0) * desiredChannels;
                    unsigned char* dstRow =
                        tile.data() + (y * tileW) * desiredChannels;

                    std::memcpy(dstRow, srcRow, (std::size_t)tileStride);
                }

                char outName[512];
                std::snprintf(outName, sizeof(outName), "%s%02d.png",
                    prefix.c_str(), index);

                if (!stbi_write_png(outName, tileW, tileH, desiredChannels,
                    tile.data(), tileStride)) {
                    std::printf("Failed to write tile %d to '%s'\n", index, outName);
                }
                else {
                    std::printf("Wrote tile %d -> '%s'\n", index, outName);
                }

                ++index;
            }
        }

        stbi_image_free(pixels);
        std::printf("SpriteSheet export done: %d tiles.\n", index);
    }

    void SpriteSheet_SetTexturePath(const char* path)
    {
        if (!path) return;

        std::strncpy(gSpritePath, path, sizeof(gSpritePath));
        gSpritePath[sizeof(gSpritePath) - 1] = '\0';


        gRows = 4;
        gCols = 6;
        gSelectedFrame = 0;
    }

    void RenderSpriteSheetPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected)
    {
        EnsureDefaultClips();

        if (!ImGui::Begin("Sprite Sheet"))
        {
            ImGui::End();
            return;
        }

       
        ImGui::InputText("Texture path", gSpritePath, IM_ARRAYSIZE(gSpritePath));
        ImGui::InputInt("Rows", &gRows);
        ImGui::InputInt("Cols", &gCols);
        gRows = std::max(gRows, 1);
        gCols = std::max(gCols, 1);
        if (ImGui::Button("Export tiles to PNGs")) {
            ExportCurrentSpritesheetToPNGs();
            {
                using namespace CrashoutsEngine::Assets;
                GenerateAssetManifest();                          
                AssetManager::GetInstance().LoadManifestAssets(); 
            }
        }
        ImGui::SliderFloat("Zoom", &gZoom, 0.5f, 6.0f);

        
        GLuint tex = TextureRenderer::GetTextureHandle(gSpritePath);

        if (tex == 0)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                "Texture not found or not loaded.");
            ImGui::End();
            return;
        }

       
        ImVec2 previewSize(192.0f * gZoom, 128.0f * gZoom);
        ImVec2 topLeft = ImGui::GetCursorScreenPos();

        ImGui::Image((ImTextureID)(uintptr_t)tex, previewSize);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float cellW = previewSize.x / gCols;
        float cellH = previewSize.y / gRows;

       
        for (int r = 0; r <= gRows; ++r)
        {
            float y = topLeft.y + r * cellH;
            drawList->AddLine(ImVec2(topLeft.x, y),
                ImVec2(topLeft.x + previewSize.x, y),
                IM_COL32(255, 255, 255, 60));
        }
        for (int c = 0; c <= gCols; ++c)
        {
            float x = topLeft.x + c * cellW;
            drawList->AddLine(ImVec2(x, topLeft.y),
                ImVec2(x, topLeft.y + previewSize.y),
                IM_COL32(255, 255, 255, 60));
        }

      
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            float localX = mouse.x - topLeft.x;
            float localY = mouse.y - topLeft.y;

            int col = (int)(localX / cellW);
            int row = (int)(localY / cellH);

            if (col >= 0 && col < gCols && row >= 0 && row < gRows)
            {
                gSelectedFrame = row * gCols + col;
            }
        }

        int selRow = gSelectedFrame / gCols;
        int selCol = gSelectedFrame % gCols;
        if (selRow >= 0 && selRow < gRows && selCol >= 0 && selCol < gCols)
        {
            ImVec2 p0(topLeft.x + selCol * cellW, topLeft.y + selRow * cellH);
            ImVec2 p1(p0.x + cellW, p0.y + cellH);
            drawList->AddRect(p0, p1, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }

        ImGui::Text("Selected frame: %d  (row %d, col %d)",
            gSelectedFrame, selRow, selCol);
        
        if (selRow >= 0 && selRow < gRows && selCol >= 0 && selCol < gCols)
        {
          
            float u0 = (float)selCol / (float)gCols;
            float u1 = (float)(selCol + 1) / (float)gCols;
            float v0 = (float)selRow / (float)gRows;
            float v1 = (float)(selRow + 1) / (float)gRows;

            ImGui::Separator();
            ImGui::Text("Selected frame preview:");

            ImVec2 previewTileSize(cellW * 2.0f, cellH * 2.0f);
            ImGui::Image((ImTextureID)(uintptr_t)tex,
                previewTileSize,
                ImVec2(u0, v0),   
                ImVec2(u1, v1));  
        }

        ImGui::SeparatorText("Clips (like Unity)");

        // ================== CLIP LIST =======================================
        for (std::size_t i = 0; i < gClips.size(); ++i)
        {
            ImGui::PushID((int)i);
            SpriteClipUI& clip = gClips[i];

            ImGui::InputText("Name", clip.name, IM_ARRAYSIZE(clip.name));
            ImGui::InputInt("Start frame", &clip.startFrame);
            ImGui::InputInt("Frame count", &clip.frameCount);
            ImGui::InputFloat("FPS", &clip.fps);
            ImGui::Checkbox("Loop", &clip.loop);

            clip.startFrame = std::max(clip.startFrame, 0);
            clip.frameCount = std::max(clip.frameCount, 1);

            if (ImGui::Button("Use selected as start"))
            {
                clip.startFrame = gSelectedFrame;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete clip"))
            {
                gClips.erase(gClips.begin() + (long long)i);
                ImGui::PopID();
                break;
            }

            ImGui::Separator();
            ImGui::PopID();
        }
    
       
        if (selected) {
            bool hasSheet = ecs.HasComponent<SpriteSheetComponent>(selected);
            bool hasAnim = ecs.HasComponent<SpriteAnimComponent>(selected);

            ImGui::Separator();
            ImGui::Text("Selected entity: %u", (unsigned)selected);
            ImGui::Text("Has SpriteSheetComponent: %s", hasSheet ? "YES" : "NO");
            ImGui::Text("Has SpriteAnimComponent:  %s", hasAnim ? "YES" : "NO");
        }

        if (ImGui::Button("Apply selected frame to entity"))
        {

            CEVec2 worldSize{ 25.0f, 25.0f }; 
            Component::SetSpriteSheet(selected, gSpritePath, gCols, gRows, worldSize);

          
            auto* anim = ecs.GetComponent<Component::SpriteAnimComponent>(selected);
            if (!anim)
            {
                Component::SpriteAnimComponent newAnim{};
                newAnim.startFrame = gSelectedFrame;
                newAnim.frameCount = 1;
                newAnim.frameTime = 0.1f;
                newAnim.frameIndex = 0;
                newAnim.timer = 0.0f;
                newAnim.flip = false;
                ecs.AddComponent<Component::SpriteAnimComponent>(selected, newAnim);
            }
            else
            {
                anim->startFrame = gSelectedFrame;
                anim->frameCount = 1;
                anim->frameTime = 0.1f;
                anim->frameIndex = 0;
                anim->timer = 0.0f;
            }
        }



        if (ImGui::Button("Add new clip"))
        {
            SpriteClipUI c{};
            std::strcpy(c.name, "NewClip");
            c.startFrame = gSelectedFrame;
            c.frameCount = 1;
            c.fps = 10.0f;
            c.loop = true;
            gClips.push_back(c);
        }

        ImGui::End();
    }

} 
