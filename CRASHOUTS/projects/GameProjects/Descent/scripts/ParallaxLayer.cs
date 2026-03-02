/*!
@file       ParallaxLayer.cs
@author     lee.r@digipen.edu
@date       04/02/2026
@brief      Implements parallax scrolling for 2D platformer backgrounds.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class ParallaxLayer : ScriptBase
    {
        // --- SETTINGS ---
        // 1.0 = Moves exactly with Cam (Pinned to screen)
        // 0.0 = Static in world (Normal objects)
        // 0.9 = Far Background (Moves slowly relative to Cam)
        // -0.5 = Foreground (Moves faster than Cam)
        [SerializeField] public float bgFactor = 0.9f;
        [SerializeField] public float midFactor = 0.5f;
        [SerializeField] public float fgFactor = -0.3f;

        [SerializeField] public bool lockY = true; // Keep background at original Y height?
        [SerializeField] public string targetName = "Camera"; // Name of the entity to follow

        // --- STATE ---
        private Entity CamEntity;
        private TransformComponent myTransform;
        private TransformComponent CamTransform;
        private TagComponent myTag;

        private float startPosX;
        private float startPosY;
        private float startCamX;
        private float startCamY;

        private float currentFactor = 0.0f;

        public override void Start()
        {
            // 1. GET COMPONENTS
            myTransform = Entity.GetComponent<TransformComponent>();
            myTag = Entity.GetComponent<TagComponent>();

            // 2. FIND Cam (The "Camera")
            CamEntity = Entity.Find(targetName);

            if (CamEntity != null)
            {
                CamTransform = CamEntity.GetComponent<TransformComponent>();

                // Cache starting positions for relative calculation
                startCamX = CamTransform.Position.x;
                startCamY = CamTransform.Position.y;
            }
            else
            {
                Console.WriteLine("[Parallax] Error: Could not find Camera entity!");
            }

            // 3. CACHE MY STARTING POS
            startPosX = myTransform.Position.x;
            startPosY = myTransform.Position.y;

            // 4. DETERMINE PARALLAX FACTOR BY TAG
            if (myTag != null)
            {
                if (myTag.HasTag("Background"))
                {
                    currentFactor = bgFactor;
                }
                else if (myTag.HasTag("Middleground"))
                {
                    currentFactor = midFactor;
                }
                else if (myTag.HasTag("Foreground"))
                {
                    currentFactor = fgFactor;
                }
                else
                {
                    currentFactor = 0.0f;
                }
            }
        }

        public override void LateUpdate()
        {
            if (CamTransform == null) return;

            // 1. CALCULATE DISTANCE MOVED
            // How far has the Cam moved from the start of the scene?
            float distMovedX = CamTransform.Position.x - startCamX;
            float distMovedY = CamTransform.Position.y - startCamY;

            // 2. CALCULATE PARALLAX OFFSET
            // If factor is 0.9, we move 0.9 units for every 1 unit the Cam moves.
            // This means the Cam "outruns" the background by 0.1 units, creating depth.
            float parallaxX = distMovedX * currentFactor;
            float parallaxY = distMovedY * currentFactor;

            // 3. APPLY NEW POSITION
            float newX = startPosX + parallaxX;

            // If lockY is true, stay at original Y. 
            // If false, apply parallax to Y as well (good for vertical platforming).
            float newY = lockY ? startPosY : (startPosY + parallaxY);

            myTransform.Position = new CEVec3(newX, newY, myTransform.Position.z);
        }
    }
}
