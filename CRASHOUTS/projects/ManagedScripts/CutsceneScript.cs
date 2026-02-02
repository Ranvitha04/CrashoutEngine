using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CutsceneScript : ScriptBase
    {
        public Entity Entity;

        public override void Start() 
        { 
            // Called once on start
        }

        public override void Update() 
        {
            // Called every frame
            if (Input.IsKeyPressed(Key.K))
            {
                Debug.Log("Transitifbfhdon");
                SceneManager.TransitionToScene("", 1.0f);
            }
        }

        public override void FixedUpdate() 
        { 
            // Called every physics step
        }
    }
}
