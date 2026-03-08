using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class GhostController : ScriptBase
    {
        public string TrackName = "Level1";

        public override void Start()
        {
            bool ghostLoaded = ReplayManager.LoadRecording($"{TrackName}_Ghost");
            if (!ghostLoaded)
            {
                InternalCalls.SetActive(Entity.ID, false);
                return;
            }
            ReplayManager.StartPlayback();
        }

        public override void FixedUpdate()
        {
            if (ReplayManager.GetGhostTransform(out CEVec3 savedPos, out float savedRot))
            {
                // GET THE LIVE TRANSFORM
                var liveTransform = Entity.GetComponent<TransformComponent>();
                Debug.Log($"GHOST BRAIN: Trying to move to X: {savedPos.x}, Y: {savedPos.y}");
                // Set the position directly!
                liveTransform.Position = savedPos;
                liveTransform.Rotation = savedRot;
            }
        }
    }
}