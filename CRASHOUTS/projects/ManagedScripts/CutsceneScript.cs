using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CutsceneScript : ScriptBase
    {

        Entity dialogue_1 = null;
        Entity dialogue_2 = null;
        Entity dialogue_3 = null;
        int currentDialogue = 1;
        bool dialoguePlaying = false;

        float timer = 0.0f;
        // Hard coding :(
        float delay_1 = 10.0f;
        float delay_2 = 10.0f;
        float delay_3 = 10.0f;
                           
        public override void Start() 
        {
            // Called once on start
            //SceneManager.Fade(0.0f, 1.0f);
            dialogue_1 = Entity.Find("Dialogue_1");
            dialogue_2 = Entity.Find("Dialogue_2");
            dialogue_3 = Entity.Find("Dialogue_3");
            dialoguePlaying = false;
        }

        public override void Update() 
        {
            //if (dialogue_1 != null) { Debug.Log("FOUND"); }
            //else { Debug.Log("NOT FOUND"); }
            // Called every frame
            //if (SceneManager.IsTransitioning())
            //{
            //    Debug.Log("Transitioning");
            //}
            //else
            //{
            //    Debug.Log("Not Transitioning");
            //}

            if (timer > 0.0f)
            {
                Debug.Log("timer is " + timer.ToString());
                timer -= (float) InternalCalls.TimeGetDT();
                if (timer <= 0.0f) { 
                    // IF LAST!
                    if(currentDialogue == 3) { SceneManager.LoadScene("tutorial", 2.0f); }
                    
                    currentDialogue++;
                    dialoguePlaying = false;
                }
            }


            if (!SceneManager.IsTransitioning() && !dialoguePlaying)
            {
                dialoguePlaying = true;
                switch (currentDialogue)
                {
                    case 1:
                        if (!InternalCalls.GetActive(dialogue_1.ID))
                        {
                            if(InternalCalls.GetActive(dialogue_2.ID)) { InternalCalls.SetActive(dialogue_2.ID, false); }
                            if(InternalCalls.GetActive(dialogue_3.ID)) { InternalCalls.SetActive(dialogue_3.ID, false); }

                            Debug.Log("Dialogue 1 not active");
                            InternalCalls.SetActive(dialogue_1.ID, true);
                            timer = delay_1;
                            Debug.Log("Set timer!");
                        }
                        break;
                    case 2:
                        if (!InternalCalls.GetActive(dialogue_2.ID))
                        {
                            if (InternalCalls.GetActive(dialogue_1.ID)) { InternalCalls.SetActive(dialogue_1.ID, false); }
                            if (InternalCalls.GetActive(dialogue_3.ID)) { InternalCalls.SetActive(dialogue_3.ID, false); }

                            Debug.Log("Dialogue 2 not active");
                            InternalCalls.SetActive(dialogue_2.ID, true);
                            timer = delay_2;
                        }
                        break;
                    case 3:
                        if (!InternalCalls.GetActive(dialogue_3.ID))
                        {
                            if (InternalCalls.GetActive(dialogue_2.ID)) { InternalCalls.SetActive(dialogue_2.ID, false); }
                            if (InternalCalls.GetActive(dialogue_1.ID)) { InternalCalls.SetActive(dialogue_1.ID, false); }

                            Debug.Log("Dialogue 3 not active");
                            InternalCalls.SetActive(dialogue_3.ID, true);
                            timer = delay_3;
                        }
                        break;
                }
            }

            if (Input.IsKeyTriggered(Key.Space))
            {
                // IF LAST
                if(currentDialogue == 3) SceneManager.LoadScene("tutorial", 2.0f);
                else
                {
                    currentDialogue++;
                    dialoguePlaying = false;
                }
            }
        }

        public override void FixedUpdate() 
        { 
            // Called every physics step
        }
    }
}
