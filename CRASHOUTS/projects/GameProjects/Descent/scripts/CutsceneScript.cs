/*!
@file       CutsceneScript.cs
@author     Renfred Lee
@date       02/02/2026
@brief      Script to manage cutscene sequences with images and dialogues.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CutsceneScript : ScriptBase
    {
        // Entities
        Entity image_1, image_2, image_3, image_4;
        Entity dialogue_1_1, dialogue_1_2, dialogue_1_3, dialogue_1_4, dialogue_1_5;
        Entity dialogue_2_1, dialogue_2_2, dialogue_2_3, dialogue_2_4, dialogue_2_5;
        Entity dialogue_3_1, dialogue_3_2, dialogue_3_3, dialogue_3_4, dialogue_4_1;

        // State
        int currentDialogue = 1;
        int currentImage = 1;
        bool dialoguePlaying = false;

        // Transitions
        bool waitingForFadeToSwap = false;
        bool waitingForFadeIn = false;
        bool waitingForPreDialogueDelay = false; // NEW: Specific wait before text appears

        // Gatekeeper
        bool hasCutsceneStarted = false;

        // Timing
        float timer = 0.0f;
        float delay = 2.5f;

        // Settings
        float fadeDuration = 1.0f;
        float fadeWaitTimer = 0.0f;

        public override void Start()
        {
            // --- FIND REFERENCES ---
            image_1 = Entity.Find("Cutscene_1");
            image_2 = Entity.Find("Cutscene_2");
            image_3 = Entity.Find("Cutscene_3");
            image_4 = Entity.Find("Cutscene_4");

            dialogue_1_1 = Entity.Find("Dialogue_1_1");
            dialogue_1_2 = Entity.Find("Dialogue_1_2");
            dialogue_1_3 = Entity.Find("Dialogue_1_3");
            dialogue_1_4 = Entity.Find("Dialogue_1_4");
            dialogue_1_5 = Entity.Find("Dialogue_1_5");

            dialogue_2_1 = Entity.Find("Dialogue_2_1");
            dialogue_2_2 = Entity.Find("Dialogue_2_2");
            dialogue_2_3 = Entity.Find("Dialogue_2_3");
            dialogue_2_4 = Entity.Find("Dialogue_2_4");
            dialogue_2_5 = Entity.Find("Dialogue_2_5");

            dialogue_3_1 = Entity.Find("Dialogue_3_1");
            dialogue_3_2 = Entity.Find("Dialogue_3_2");
            dialogue_3_3 = Entity.Find("Dialogue_3_3");
            dialogue_3_4 = Entity.Find("Dialogue_3_4");

            dialogue_4_1 = Entity.Find("Dialogue_4_1");

            // Init State
            dialoguePlaying = false;
            waitingForFadeToSwap = false;
            waitingForFadeIn = false;
            waitingForPreDialogueDelay = false;
            hasCutsceneStarted = false;

            currentDialogue = 1;
            currentImage = 1;

            DeactivateAllImages();
            DeactivateAllDialogues();
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
                if (dialogue_1_1 != null) InternalCalls.SetActive(dialogue_1_1.ID, true);
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
                    Debug.Log("SWAP");
                    currentImage++;
                    currentDialogue = 1;

                    DeactivateAllImages();
                    DeactivateAllDialogues();

                    Entity nextImg = GetImageEntity(currentImage);
                    if (nextImg != null) InternalCalls.SetActive(nextImg.ID, true);

                    // Move to Phase B (Fade In)
                    waitingForFadeToSwap = false;
                    waitingForFadeIn = true;
                }
                return;
            }

            // =========================================================
            // 2. PHASE B: WAIT FOR FADE IN 
            // =========================================================
            if (waitingForFadeIn)
            {
                if (SceneManager.IsTransitioning()) return; // Wait...

                // -- FADE IN COMPLETE --

                // NEW LOGIC: Is this the 4th Image?
                if (currentImage == 4)
                {
                    // Yes: Do NOT show text yet. Wait 1.5s first.
                    timer = 1.5f;
                    waitingForPreDialogueDelay = true;
                    waitingForFadeIn = false;
                    return;
                }

                // No: Normal behavior (Show text immediately)
                Entity nextDia = GetDialogueEntity(currentImage, 1);
                if (nextDia != null) InternalCalls.SetActive(nextDia.ID, true);

                timer = delay;
                waitingForFadeIn = false;
                dialoguePlaying = false;
                return;
            }

            // =========================================================
            // 3. PHASE C: PRE-DIALOGUE DELAY (For Image 4)
            // =========================================================
            if (waitingForPreDialogueDelay)
            {
                timer -= (float)InternalCalls.TimeGetDT();

                if (timer <= 0.0f)
                {
                    // 1.5 Seconds have passed. Screen is visible.

                    // ---------------------------------------------------------
                    // TODO: CALL AUDIO PLAY FUNCTION HERE
                    // e.g. AudioManager.Play("dramatic_reveal");
                    AudioManager.GetInstance().PlayAttached("PortalSFX", image_4.ID);
                    // ---------------------------------------------------------

                    // Show the text
                    Entity nextDia = GetDialogueEntity(currentImage, 1);
                    if (nextDia != null) InternalCalls.SetActive(nextDia.ID, true);

                    // Reset for reading time
                    timer = delay;
                    waitingForPreDialogueDelay = false;
                    dialoguePlaying = false; // Let normal update loop take over
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
            // Skip whole cutscene
            if (Input.IsKeyTriggered(Key.Escape))
            {
                SceneManager.LoadScene("tutorial", 2.0f);
            }

            // =========================================================
            // 6. SAFETY UPDATE LOOP
            // =========================================================
            // Add !waitingForPreDialogueDelay to the check so we don't accidentally run this during the 1.5s wait
            if (!SceneManager.IsTransitioning() && !dialoguePlaying &&
                !waitingForFadeToSwap && !waitingForFadeIn && !waitingForPreDialogueDelay)
            {
                dialoguePlaying = true;

                DeactivateAllDialogues();

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
                    timer = 1.0f;
                }
            }
        }

        // --- LOGIC ---

        private void AdvanceDialogue()
        {
            timer = 0.0f;
            dialoguePlaying = false;

            int maxDialogue = 1;
            switch (currentImage)
            {
                case 1: maxDialogue = 5; break;
                case 2: maxDialogue = 5; break;
                case 3: maxDialogue = 4; break;
                case 4: maxDialogue = 1; break;
            }

            if (currentDialogue < maxDialogue)
            {
                currentDialogue++;
            }
            else
            {
                if (currentImage < 4)
                {
                    SceneManager.Fade(fadeDuration, 1.0f);
                    fadeWaitTimer = fadeDuration;
                    waitingForFadeToSwap = true;
                }
                else
                {
                    SceneManager.LoadScene("tutorial", 2.0f);
                }
            }
        }

        // ... [Helpers remain the same] ...
        private Entity GetDialogueEntity(int img, int dia)
        {
            if (img == 1)
            {
                if (dia == 1) return dialogue_1_1;
                if (dia == 2) return dialogue_1_2;
                if (dia == 3) return dialogue_1_3;
                if (dia == 4) return dialogue_1_4;
                if (dia == 5) return dialogue_1_5;
            }
            else if (img == 2)
            {
                if (dia == 1) return dialogue_2_1;
                if (dia == 2) return dialogue_2_2;
                if (dia == 3) return dialogue_2_3;
                if (dia == 4) return dialogue_2_4;
                if (dia == 5) return dialogue_2_5;
            }
            else if (img == 3)
            {
                if (dia == 1) return dialogue_3_1;
                if (dia == 2) return dialogue_3_2;
                if (dia == 3) return dialogue_3_3;
                if (dia == 4) return dialogue_3_4;
            }
            else if (img == 4)
            {
                if (dia == 1) return dialogue_4_1;
            }
            return null;
        }

        private Entity GetImageEntity(int img)
        {
            if (img == 1) return image_1;
            if (img == 2) return image_2;
            if (img == 3) return image_3;
            if (img == 4) return image_4;
            return null;
        }

        private void DeactivateAllDialogues()
        {
            InternalCalls.SetActive(dialogue_1_1.ID, false);
            InternalCalls.SetActive(dialogue_1_2.ID, false);
            InternalCalls.SetActive(dialogue_1_3.ID, false);
            InternalCalls.SetActive(dialogue_1_4.ID, false);
            InternalCalls.SetActive(dialogue_1_5.ID, false);

            InternalCalls.SetActive(dialogue_2_1.ID, false);
            InternalCalls.SetActive(dialogue_2_2.ID, false);
            InternalCalls.SetActive(dialogue_2_3.ID, false);
            InternalCalls.SetActive(dialogue_2_4.ID, false);
            InternalCalls.SetActive(dialogue_2_5.ID, false);

            InternalCalls.SetActive(dialogue_3_1.ID, false);
            InternalCalls.SetActive(dialogue_3_2.ID, false);
            InternalCalls.SetActive(dialogue_3_3.ID, false);
            InternalCalls.SetActive(dialogue_3_4.ID, false);

            InternalCalls.SetActive(dialogue_4_1.ID, false);
        }

        private void DeactivateAllImages()
        {
            if (image_1 != null) InternalCalls.SetActive(image_1.ID, false);
            if (image_2 != null) InternalCalls.SetActive(image_2.ID, false);
            if (image_3 != null) InternalCalls.SetActive(image_3.ID, false);
            if (image_4 != null) InternalCalls.SetActive(image_4.ID, false);
        }
    }
}
