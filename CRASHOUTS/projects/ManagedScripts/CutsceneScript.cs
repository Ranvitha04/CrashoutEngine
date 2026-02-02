using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CutsceneScript : ScriptBase
    {

        public override void Start() 
        {
            // Called once on start
            //SceneManager.Fade(0.0f, 1.0f);
        }

        public override void Update() 
        {
            // Called every frame
            if (SceneManager.IsTransitioning())
            {
                Debug.Log("Transitioning");
            }
            else
            {
                Debug.Log("Not Transitioning");
            }

            if (Input.IsKeyTriggered(Key.Space))
            {
                SceneManager.LoadScene("tutorial", 2.0f);
            }
        }

        public override void FixedUpdate() 
        { 
            // Called every physics step
        }
    }
}
