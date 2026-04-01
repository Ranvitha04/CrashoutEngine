/*!
@file       EndCutsceneScript.cs
@author     Renfred (RTIS)
@date       02/02/2026
@brief      Script to manage the final cutscene sequence with images and dialogues.

All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class EndCutsceneScript : ScriptBase
    {
        // Entities
        Entity image_1, image_2, image_3, image_4, image_5, image_6;
        Entity black;
        Entity creditsContainer; // NEW: Parent object holding text and splash images
        
        // Dialogues mapped to the JSON
        Entity dialogue_2_1;
        Entity dialogue_5_1, dialogue_5_2, dialogue_5_3, dialogue_5_4;
        Entity dialogue_6_1, dialogue_6_2;

        // State
        int currentDialogue = 1;
        int currentImage = 1;
        bool dialoguePlaying = false;

        // Transitions
        bool waitingForFadeToSwap = false;
        bool waitingForFadeIn = false;
        bool waitingForCredits = false; 

        // Timing & Settings
        float timer = 0.0f;
        float delay = 2.5f;
        float fadeDuration = 1.0f;
        float fadeWaitTimer = 0.0f;

        // Credits Settings
        public float creditsDuration = 5.0f; 
        public float creditsScrollSpeed = 100.0f; // NEW: How fast the container moves UP (units per second)

        // Gatekeeper
        bool hasCutsceneStarted = false;

        public override void Start()
        {
            // --- FIND REFERENCES ---
            image_1 = Entity.Find("Cutscene_1");
            image_2 = Entity.Find("Cutscene_2");
            image_3 = Entity.Find("Cutscene_3");
            image_4 = Entity.Find("Cutscene_4");
            image_5 = Entity.Find("Cutscene_5");
            image_6 = Entity.Find("Cutscene_6");
            
            black = Entity.Find("BLACK");
            creditsContainer = Entity.Find("CreditsContainer"); // Parent object for text and splash images

            dialogue_2_1 = Entity.Find("Dialogue_2_1");

            dialogue_5_1 = Entity.Find("Dialogue_5_1");
            dialogue_5_2 = Entity.Find("Dialogue_5_2");
            dialogue_5_3 = Entity.Find("Dialogue_5_3");
            dialogue_5_4 = Entity.Find("Dialogue_5_4");

            dialogue_6_1 = Entity.Find("Dialogue_6_1");
            dialogue_6_2 = Entity.Find("Dialogue_6_2");

            // Init State
            dialoguePlaying = false;
            waitingForFadeToSwap = false;
            waitingForFadeIn = false;
            waitingForCredits = false;
            hasCutsceneStarted = false;

            currentDialogue = 1;
            currentImage = 1;

            DeactivateAllImages();
            DeactivateAllDialogues();
            
            if (black != null) InternalCalls.SetActive(black.ID, false);
            if (creditsContainer != null) InternalCalls.SetActive(creditsContainer.ID, false);
            
            if (image_1 != null) InternalCalls.SetActive(image_1.ID, true);
        }

        public override void Update()
        {
            // =========================================================
            // 0. GATEKEEPER (First Load)
            // =========================================================
            if (!hasCutsceneStarted)
            {
                if (SceneManager.IsTransitioning()) return;

                hasCutsceneStarted = true;
                timer = delay;
                dialoguePlaying = true;
                return;
            }

            // =========================================================
            // 1. PHASE A: WAIT FOR BLACK SCREEN (Swap Images)
            // =========================================================
            if (waitingForFadeToSwap)
            {
                fadeWaitTimer -= (float)InternalCalls.TimeGetDT();

                if (fadeWaitTimer <= 0.0f)
                {
                    currentImage++;
                    currentDialogue = 1;

                    DeactivateAllImages();
                    
                    if (currentImage != 3 && currentImage != 4)
                    {
                        DeactivateAllDialogues();
                    }

                    if (currentImage <= 6)
                    {
                        // Normal sequence transition
                        Entity nextImg = GetImageEntity(currentImage);
                        if (nextImg != null) InternalCalls.SetActive(nextImg.ID, true);

                        waitingForFadeToSwap = false;
                        waitingForFadeIn = true;
                    }
                    else 
                    {
                        // Enter Credits Sequence
                        if (black != null) InternalCalls.SetActive(black.ID, true);
                        if (creditsContainer != null) InternalCalls.SetActive(creditsContainer.ID, true);

                        timer = creditsDuration; 
                        waitingForFadeToSwap = false;
                        waitingForCredits = true;
                    }
                }
                return;
            }

            // =========================================================
            // 2. PHASE B: WAIT FOR FADE IN 
            // =========================================================
            if (waitingForFadeIn)
            {
                if (SceneManager.IsTransitioning()) return;

                Entity nextDia = GetDialogueEntity(currentImage, 1);
                if (nextDia != null) InternalCalls.SetActive(nextDia.ID, true);

                timer = delay;
                waitingForFadeIn = false;
                dialoguePlaying = false;
                return;
            }

            // =========================================================
            // 3. PHASE C: CREDITS SCROLLING & WAIT 
            // =========================================================
            if (waitingForCredits)
            {
                if (SceneManager.IsTransitioning()) return; 

                float dt = (float)InternalCalls.TimeGetDT();

                // --- SCROLL THE CREDITS UPWARDS ---
                if (creditsContainer != null)
                {
                    var containerTransform = creditsContainer.GetComponent<TransformComponent>();
                    if (containerTransform != null)
                    {
                        // Add to the Y position based on speed and delta time
                        containerTransform.Position = new CEVec3(
                            containerTransform.Position.x, 
                            containerTransform.Position.y + (creditsScrollSpeed * dt), 
                            containerTransform.Position.z
                        );
                    }
                }

                // --- TIMER LOGIC ---
                timer -= dt;
                if (timer <= 0.0f)
                {
                    SceneManager.LoadScene("descentmenu", 2.0f);
                    waitingForCredits = false; 
                }
                
                // Allow skipping
                if (Input.IsKeyTriggered(Key.Space) || Input.IsKeyTriggered(Key.Escape))
                {
                    SceneManager.LoadScene("descentmenu", 2.0f);
                    waitingForCredits = false;
                }
                return;
            }

            // =========================================================
            // 4. NORMAL TIMER LOGIC (Typewriter & Reading)
            // =========================================================
            if (timer > 0.0f)
            {
                Entity activeDia = GetDialogueEntity(currentImage, currentDialogue);
                bool isTypingDone = (activeDia == null) ? true : InternalCalls.IsTextFinished(activeDia.ID);

                if (isTypingDone)
                {
                    timer -= (float)InternalCalls.TimeGetDT();
                }

                if (timer <= 0.0f)
                {
                    AdvanceDialogue();
                }
            }

            // =========================================================
            // 5. INPUT LOGIC
            // =========================================================
            if (Input.IsKeyTriggered(Key.Space))
            {
                Entity activeDia = GetDialogueEntity(currentImage, currentDialogue);
                bool isTypingDone = (activeDia == null) ? true : InternalCalls.IsTextFinished(activeDia.ID);

                if (!isTypingDone) AdvanceDialogue(); 
                else AdvanceDialogue();
            }
            
            if (Input.IsKeyTriggered(Key.Escape))
            {
                SceneManager.LoadScene("descentmenu", 2.0f); 
            }

            // =========================================================
            // 6. SAFETY UPDATE LOOP
            // =========================================================
            if (!SceneManager.IsTransitioning() && !dialoguePlaying &&
                !waitingForFadeToSwap && !waitingForFadeIn && !waitingForCredits)
            {
                dialoguePlaying = true;

                if (currentImage != 3 && currentImage != 4)
                {
                    DeactivateAllDialogues();
                }

                Entity currentImgEnt = GetImageEntity(currentImage);
                if (currentImgEnt != null) InternalCalls.SetActive(currentImgEnt.ID, true);

                Entity currentDiaEnt = GetDialogueEntity(currentImage, currentDialogue);
                if (currentDiaEnt != null)
                {
                    InternalCalls.SetActive(currentDiaEnt.ID, true);
                    timer = delay;
                }
                else
                {
                    timer = delay;
                }
            }
        }

        // --- LOGIC ---

        private void AdvanceDialogue()
        {
            timer = 0.0f;
            dialoguePlaying = false;

            int maxDialogue = 0;
            switch (currentImage)
            {
                case 1: maxDialogue = 0; break;
                case 2: maxDialogue = 1; break;
                case 3: maxDialogue = 0; break; 
                case 4: maxDialogue = 0; break; 
                case 5: maxDialogue = 4; break;
                case 6: maxDialogue = 2; break;
            }

            if (currentDialogue < maxDialogue)
            {
                currentDialogue++;
            }
            else
            {
                if (currentImage <= 6)
                {
                    SceneManager.Fade(fadeDuration, 1.0f);
                    fadeWaitTimer = fadeDuration;
                    waitingForFadeToSwap = true;
                }
            }
        }

        private Entity GetDialogueEntity(int img, int dia)
        {
            if (img == 2) { if (dia == 1) return dialogue_2_1; }
            else if (img == 5)
            {
                if (dia == 1) return dialogue_5_1;
                if (dia == 2) return dialogue_5_2;
                if (dia == 3) return dialogue_5_3;
                if (dia == 4) return dialogue_5_4;
            }
            else if (img == 6)
            {
                if (dia == 1) return dialogue_6_1;
                if (dia == 2) return dialogue_6_2;
            }
            return null;
        }

        private Entity GetImageEntity(int img)
        {
            if (img == 1) return image_1;
            if (img == 2) return image_2;
            if (img == 3) return image_3;
            if (img == 4) return image_4;
            if (img == 5) return image_5;
            if (img == 6) return image_6;
            return null;
        }

        private void DeactivateAllDialogues()
        {
            if (dialogue_2_1 != null) InternalCalls.SetActive(dialogue_2_1.ID, false);
            if (dialogue_5_1 != null) InternalCalls.SetActive(dialogue_5_1.ID, false);
            if (dialogue_5_2 != null) InternalCalls.SetActive(dialogue_5_2.ID, false);
            if (dialogue_5_3 != null) InternalCalls.SetActive(dialogue_5_3.ID, false);
            if (dialogue_5_4 != null) InternalCalls.SetActive(dialogue_5_4.ID, false);
            if (dialogue_6_1 != null) InternalCalls.SetActive(dialogue_6_1.ID, false);
            if (dialogue_6_2 != null) InternalCalls.SetActive(dialogue_6_2.ID, false);
        }

        private void DeactivateAllImages()
        {
            if (image_1 != null) InternalCalls.SetActive(image_1.ID, false);
            if (image_2 != null) InternalCalls.SetActive(image_2.ID, false);
            if (image_3 != null) InternalCalls.SetActive(image_3.ID, false);
            if (image_4 != null) InternalCalls.SetActive(image_4.ID, false);
            if (image_5 != null) InternalCalls.SetActive(image_5.ID, false);
            if (image_6 != null) InternalCalls.SetActive(image_6.ID, false);
        }
    }
}