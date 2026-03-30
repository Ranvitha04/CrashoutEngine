using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class GameOverMenu : ScriptBase
    {
        private const string MenuControllerName = "MenuScript";
        private const string DefeatControllerName = "DefeatScript";

        [SerializeField] public Entity RestartBtn;
        [SerializeField] public Entity QuitBtn;
        [SerializeField] private string quitScene = "descentmenu";
        [SerializeField] private float quitTransition = 0.5f;

        private int selectedButtonIndex = 0;
        private float navigationCooldown = 0.0f;
        private bool controllerNavigationActive = false;

        public override void Start()
        {
            if (RestartBtn == null || RestartBtn.ID == 0)
            {
                RestartBtn = Entity.Find("RestartBtn");
            }

            if (RestartBtn == null || RestartBtn.ID == 0)
            {
                RestartBtn = Entity.Find("StartBtn");
            }

            if (QuitBtn == null || QuitBtn.ID == 0)
            {
                QuitBtn = Entity.Find("QuitBtn");
            }
            InternalCalls.UI_ClearForcedHoveredButton();
        }

        public override void Update()
        {
            if (!IsControllerEntity())
            {
                return;
            }

            if (navigationCooldown > 0.0f)
            {
                navigationCooldown -= (float)Time.DT;
            }

            Entity[] buttons = GetButtons();
            UpdateControllerSelection(buttons);

            if (IsControllerSubmit(buttons, RestartBtn))
            {
                RestartFromCheckpoint();
            }
            else if (IsControllerSubmit(buttons, QuitBtn))
            {
                SceneManager.LoadScene(quitScene, quitTransition);
            }
        }

        private Entity[] GetButtons()
        {
            System.Collections.Generic.List<Entity> buttons = new System.Collections.Generic.List<Entity>();

            if (RestartBtn != null && RestartBtn.ID != 0 && RestartBtn.IsActive())
            {
                buttons.Add(RestartBtn);
            }

            if (QuitBtn != null && QuitBtn.ID != 0 && QuitBtn.IsActive())
            {
                buttons.Add(QuitBtn);
            }

            return buttons.ToArray();
        }

        private void RestartFromCheckpoint()
        {
            PlayerMovement.RespawnNextLoad = true;
            string targetScene = PlayerMovement.lastLevelName;
            float fadeDuration = 0.5f;

            if (string.IsNullOrEmpty(targetScene))
            {
                targetScene = "tutorial";
            }

            SceneManager.LoadScene(targetScene, fadeDuration);
        }

        private void UpdateControllerSelection(Entity[] buttons)
        {
            if (buttons == null || buttons.Length == 0)
            {
                controllerNavigationActive = false;
                InternalCalls.UI_ClearForcedHoveredButton();
                return;
            }

            bool movedDown = false;
            bool movedUp = false;
            if (navigationCooldown <= 0.0f)
            {
                float menuX = Input.GetAxis("MenuX");
                if (Input.GetActionTriggered("UIRight") || menuX >= 0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex + 1) % buttons.Length;
                    navigationCooldown = 0.2f;
                    movedDown = true;
                }
                else if (Input.GetActionTriggered("UILeft") || menuX <= -0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex - 1 + buttons.Length) % buttons.Length;
                    navigationCooldown = 0.2f;
                    movedUp = true;
                }
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                selectedButtonIndex = 0;
            }

            if (movedDown || movedUp || Input.GetActionTriggered("UISubmit"))
            {
                controllerNavigationActive = true;
            }

            if (!controllerNavigationActive)
            {
                InternalCalls.UI_ClearForcedHoveredButton();
                return;
            }

            Entity selected = buttons[selectedButtonIndex];
            if (selected != null && selected.ID != 0 && selected.IsActive())
            {
                InternalCalls.UI_ClearForcedHoveredButton();
                InternalCalls.UI_SetForcedHoveredButton(selected.ID);
            }
            else
            {
                InternalCalls.UI_ClearForcedHoveredButton();
            }
        }

        private bool IsControllerSubmit(Entity[] buttons, Entity target)
        {
            if (target == null || target.ID == 0 || buttons == null)
            {
                return false;
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                return false;
            }

            return buttons[selectedButtonIndex] == target && Input.GetActionTriggered("UISubmit");
        }

        private bool IsControllerEntity()
        {
            return Entity != null &&
                   Entity.ID != 0 &&
                   (InternalCalls.GetName(Entity.ID) == MenuControllerName ||
                    InternalCalls.GetName(Entity.ID) == DefeatControllerName);
        }

        public override void OnDestroy()
        {
            if (IsControllerEntity())
            {
                InternalCalls.UI_ClearForcedHoveredButton();
            }
        }
    }
}
