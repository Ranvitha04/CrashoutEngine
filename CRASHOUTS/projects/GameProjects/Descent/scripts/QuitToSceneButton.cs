namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Handles mouse-driven quit button clicks for defeat/game over style scenes.

    Controller navigation is handled separately by a controller entity script such
    as GameOverMenu. This script keeps direct button click behaviour simple by
    only loading the configured scene when the attached button is clicked.
    */
    public class QuitToSceneButton : ScriptBase
    {
        [SerializeField] private string targetScene = "descentmenu";
        [SerializeField] private float fadeDuration = 0.5f;

        /*!
        @brief Load the configured scene when the attached button is clicked.
        */
        public override void Update()
        {
            ButtonComponent button = Entity.GetComponent<ButtonComponent>();
            if (button == null || !button.IsClicked())
            {
                return;
            }

            SceneManager.LoadScene(targetScene, fadeDuration);
        }
    }
}
