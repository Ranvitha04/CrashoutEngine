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

 
    static char  gSpritePath[256] = "images/spritesheet/ducky_2_spritesheet.png";
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

        std::string sheetPath;
        {
            namespace fs = std::filesystem;
            using namespace CrashoutsEngine::Assets;

            fs::path base = fs::path(rootPath);
            fs::path rel = fs::path(gSpritePath);
            sheetPath = (base / rel).generic_string();
        }

        unsigned char* pixels = stbi_load( sheetPath.c_str(), &width, &height, &channels, desiredChannels);
     
        if (!pixels) {
            
            return;
        }

        if (gTileW <= 0 || gTileH <= 0) {
            
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

        
        int cols = width / tileW;
        int rows = height / tileH;

        if (cols <= 0 || rows <= 0) {
            std::printf("SpriteSheet export failed: invalid rows/cols from tile size\n");
            stbi_image_free(pixels);
            return;
        }

  
        if (width % tileW != 0 || height % tileH != 0) {
            std::printf("Warning: sheet not evenly divisible by tile size; extra pixels ignored.\n");
        }

       
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
        namespace Assets = CrashoutsEngine::Assets; 

    
        fs::path outDir = fs::path(Assets::rootPath) / "images" / "spritesheet" / stem;
        std::error_code ec; 
        fs::create_directories(outDir, ec); 

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

        if (!ImGui::Begin("Sprite Sheet", 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::End();
            return;
        }

        ImGui::Text(gSpritePath);
        //ImGui::InputText("", gSpritePath, IM_ARRAYSIZE(gSpritePath));

        ImGui::SameLine();
        if (ImGui::SmallButton("Choose...")) {
            ImGui::OpenPopup("Pick Texture");
        }

        // Modal popup with a simple file browser
        if (ImGui::BeginPopupModal("Pick Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            namespace fs = std::filesystem;
            static std::vector<std::string> s_assets;
            static bool s_scanned = false;

            // Refresh List button
            if (ImGui::Button("Refresh List")) {
                s_scanned = false;
            }
            ImGui::SameLine();

            if (!s_scanned) {
                s_scanned = true;
                s_assets.clear();

                const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images"; // base folder
                for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
                    if (!p.is_regular_file()) continue;

                    const std::string ext = p.path().extension().string();
                    std::string lower = ext;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
                        lower == ".tga" || lower == ".bmp") {

                        // Store relative path for cleaner display
                        std::string rel = fs::relative(p.path(), assetRoot).string();
                        s_assets.push_back(rel);
                    }
                }

                std::sort(s_assets.begin(), s_assets.end());
            }

            // Left: list; Right: preview name
            ImGui::BeginChild("asset_list", ImVec2(420, 300), true);
            for (auto const& f : s_assets) {
                if (ImGui::Selectable(f.c_str())) {

                    std::string fullRelPath = "images/" + f;

                    std::strncpy(gSpritePath, fullRelPath.c_str(), sizeof(gSpritePath));
                    gSpritePath[sizeof(gSpritePath) - 1] = '\0'; // Ensure null termination

                    // Reset other UI states since texture changed
                    gSelectedFrame = 0;

                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::InputInt("Tile width (px)", &gTileW);
        ImGui::InputInt("Tile height (px)", &gTileH);
        gTileW = std::max(gTileW, 1);
        gTileH = std::max(gTileH, 1);
        
        int sheetW = 0, sheetH = 0, channels = 0;
      
        std::string sheetPath;
        {
            namespace fs = std::filesystem;
            using namespace CrashoutsEngine::Assets;

            fs::path base = fs::path(rootPath);    
            fs::path rel = fs::path(gSpritePath); 
            sheetPath = (base / rel).generic_string();
        }

        if (stbi_info(sheetPath.c_str(), &sheetW, &sheetH, &channels)) {
            gCols = std::max(1, sheetW / gTileW);
            gRows = std::max(1, sheetH / gTileH);

            ImGui::Text("Sheet %d x %d -> %d cols x %d rows",
                sheetW, sheetH, gCols, gRows);
        }
        else {
            
        }

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

        ImVec2 topLeft = ImGui::GetCursorScreenPos();
       
        // 1. Define a base width for the UI (e.g., 300 pixels wide)
        float baseWidth = 300.0f;

        // 2. Calculate height based on aspect ratio
        float aspectRatio = (float)sheetH / (float)sheetW;

        // 3. Create the size dynamic to the texture
        ImVec2 previewSize(baseWidth * gZoom, (baseWidth * aspectRatio) * gZoom);

        // Now the image draws with correct proportions
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
