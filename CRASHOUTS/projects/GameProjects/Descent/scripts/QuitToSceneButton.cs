namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class QuitToSceneButton : ScriptBase
    {
        [SerializeField] private string targetScene = "descentmenu";
        [SerializeField] private float fadeDuration = 0.5f;

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
