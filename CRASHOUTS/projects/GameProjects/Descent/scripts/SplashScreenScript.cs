/*!
@file       SplashScreenScript.cs
@author     Renfred Lee
@date       01/04/2026
@brief      Manages the startup splash screen sequence (DigiPen Logo -> Team Logo -> Main Menu).

All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SplashScreenScript : ScriptBase
    {
        // --- SETTINGS ---
        [SerializeField] public float logoDisplayTime = 2.5f;  // How long each logo stays on screen
        [SerializeField] public float fadeDuration = 1.0f;     // How long the fade to black takes
        [SerializeField] public string nextSceneName = "descentmenu";

        // --- ENTITIES ---
        private Entity digipenLogo;
        private Entity teamLogo;

        // --- STATE ---
        private int currentState = 0; 
        // 0 = Wait for DigiPen Logo
        // 1 = Fading out DigiPen (Screen going black)
        // 2 = Waiting for Team Logo to fade in
        // 3 = Wait for Team Logo
        // 4 = Transitioning to Menu (Done)

        private float timer = 0.0f;
        private bool hasSequenceStarted = false;

        public override void Start()
        {
            // Find references (Ensure these match your entity names in the editor!)
            digipenLogo = Entity.Find("DigipenLogo");
            teamLogo = Entity.Find("TeamLogo");

            // Initialize standard state
            if (digipenLogo != null) InternalCalls.SetActive(digipenLogo.ID, true);
            if (teamLogo != null) InternalCalls.SetActive(teamLogo.ID, false);

            currentState = 0;
            hasSequenceStarted = false;
        }

        public override void Update()
        {
            // 1. Initial Gatekeeper (Wait for the very first scene fade-in to finish)
            if (!hasSequenceStarted)
            {
                if (SceneManager.IsTransitioning()) return;
                
                hasSequenceStarted = true;
                timer = logoDisplayTime;
                return;
            }

            // 2. Allow the player to skip the intro sequence
            if (currentState < 4 && (Input.IsKeyTriggered(Key.Space) || Input.IsKeyTriggered(Key.Escape)))
            {
                SceneManager.LoadScene(nextSceneName, fadeDuration);
                currentState = 4; // Lock the state machine
                return;
            }

            // 3. State Machine Logic
            switch (currentState)
            {
                case 0: // Displaying DigiPen Logo
                    timer -= (float)Time.DT;
                    if (timer <= 0.0f)
                    {
                        // Start fading to black
                        SceneManager.Fade(fadeDuration, 1.0f);
                        timer = fadeDuration;
                        currentState = 1;
                    }
                    break;

                case 1: // Waiting for screen to be fully black
                    timer -= (float)Time.DT;
                    if (timer <= 0.0f)
                    {
                        // Screen is black! Swap the logos instantly
                        if (digipenLogo != null) InternalCalls.SetActive(digipenLogo.ID, false);
                        if (teamLogo != null) InternalCalls.SetActive(teamLogo.ID, true);

                        // The engine will now automatically fade back in, move to the next state to monitor it
                        currentState = 2;
                    }
                    break;

                case 2: // Waiting for the engine to finish fading the Team Logo in
                    if (SceneManager.IsTransitioning()) return;

                    // Fade in complete, start the display timer for the Team Logo
                    timer = logoDisplayTime;
                    currentState = 3;
                    break;

                case 3: // Displaying Team Logo
                    timer -= (float)Time.DT;
                    if (timer <= 0.0f)
                    {
                        // Time's up! Load the main menu (this handles the final fade out automatically)
                        SceneManager.LoadScene(nextSceneName, fadeDuration);
                        currentState = 4; 
                    }
                    break;

                case 4: // Loading Next Scene
                    // Do nothing, let the engine handle the scene transition.
                    break;
            }
        }
    }
}