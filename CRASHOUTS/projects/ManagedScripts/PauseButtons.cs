using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PauseButtons : ScriptBase
    {
        // Cache the ID so we don't search for "PauseScreen" every frame
        private uint _pauseScreenID = 0;
        private bool _isPaused = false;

        PlayerMovement PlayerMovement;

        public override void Start()
        {
            // 1. Find the screen ONCE at startup
            Entity screen = Entity.Find("PauseScreen");
            if (screen != null)
            {
                _pauseScreenID = screen.ID;
                
            }
            else
            {
                Console.WriteLine("[Error] PauseButtons could not find 'PauseScreen' entity!");
            }

            PlayerMovement = PlayerMovement.Instance;
        }

        public override void Update()
        {
            // 2. Use IsKeyTriggered (Runs once per press) instead of IsKeyPressed (Runs every frame while holding)
            if (Input.IsKeyTriggered(Key.Escape))
            {
                TogglePause();
            }

            // 3. Handle Buttons
            var btn = Entity.GetComponent<ButtonComponent>();
            if (btn != null && btn.IsClicked())
            {
                var nameComp = Entity.GetComponent<NameComponent>();
                string name = nameComp != null ? nameComp.ToString() : "Unknown";

                Console.WriteLine("[C#] Clicked: " + name);
                Entity screen = new Entity(_pauseScreenID);
                var tf = screen.GetComponent<TransformComponent>();

                if (tf.Position.z >= 0.0f)
                {
                    if (name == "PauseResumeBtn")
                    {
                        SetPauseMenu(false);
                    }
                    else if (name == "SettingsBtn")
                    {
                        Console.WriteLine("Settings not implemented");
                    }
                    else if (name == "PauseQuitBtn")
                    {
                        SceneManager.QuitGame();
                    }
                }
            }
        }

        void TogglePause()
        {
            SetPauseMenu(!_isPaused);
        }

        void SetPauseMenu(bool show)
        {
            if(PlayerMovement != null) PlayerMovement.isPaused = show;
            // 4. Safety Check: Make sure we actually found the screen
            if (_pauseScreenID == 0) return;

            Entity screen = new Entity(_pauseScreenID);
            var tf = screen.GetComponent<TransformComponent>();

            if (tf != null)
            {
                _isPaused = show;

                if (_isPaused)
                {
                    // Show: Bring to front (Z = 0.7f)
                    tf.Position = new CEVec3(tf.Position.x, tf.Position.y, 0.7f);
                    // Optional: Pause Physics/Time?
                    // Time.TimeScale = 0.0f; 
                }
                else
                {
                    // Hide: Send far away (Z = -100.0f)
                    // Note: Using -100 matches your Resume button logic
                    tf.Position = new CEVec3(tf.Position.x, tf.Position.y, -100.0f);
                    // Time.TimeScale = 1.0f;
                }
            }
        }
    }
}