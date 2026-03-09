/*!
@file       PlayerBookGlowController.cs
@author     
@date       07/03/2026
@brief      Keeps PlayerBookGlow positioned on the book pose and toggles it by light mode.
*/

using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerBookGlowController : ScriptBase
    {
        [SerializeField] public string bookGlowEntityName = "PlayerBookGlow";
        [SerializeField] public bool onlyGlowInLightMode = true;
        [SerializeField] public bool followZ = true;
        [SerializeField] public bool useAngledPath = true;
        [SerializeField] public float pathFirstPortion = 0.85f;
        [SerializeField] public float pathAngleDeg = 0.0f;
        [SerializeField] public float pathDistance = 60.0f;

        [SerializeField] public bool overrideStartEnd = false;
        [SerializeField] public bool mirrorXForLeft = true;
        [SerializeField] public CEVec3 startOffset = new CEVec3(-30.0f, -5.0f, 0.0f);
        [SerializeField] public CEVec3 endOffset = new CEVec3(30.0f, 0.0f, 0.0f);

        [SerializeField] public CEVec3 offsetBehindRight = new CEVec3(-30.0f, -5.0f, 0.0f);
        [SerializeField] public CEVec3 offsetBehindLeft = new CEVec3(30.0f, 0.0f, 0.0f);

        [SerializeField] public CEVec3 offsetRight = new CEVec3(30.0f, 0.0f, 0.0f);
        [SerializeField] public CEVec3 offsetLeft = new CEVec3(-30.0f, 0.0f, 0.0f);

        [SerializeField] public CEVec3 offsetRightSwitch = new CEVec3(30.0f, 0.0f, 0.0f);
        [SerializeField] public CEVec3 offsetLeftSwitch = new CEVec3(-30.0f, 0.0f, 0.0f);

        [SerializeField] public bool previewEnabled = false;
        [SerializeField] public float previewSwitchT01 = 0.0f;
        [SerializeField] public bool previewStartIsLight = false;
        [SerializeField] public bool previewTargetIsLight = true;
        [SerializeField] public bool previewFacingRight = true;
        [SerializeField] public bool previewUseSwitchClip = true;
        [SerializeField] public bool previewForceVisible = true;

        private TransformComponent playerTf;
        private Animator playerAnim;
        private PlayerMovement playerMovement;

        private Entity glowEntity;
        private TransformComponent glowTf;
        private bool lastActiveState = true;

        private static float Clamp01(float v)
        {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }

        private static float Smooth01(float t)
        {
            t = Clamp01(t);
            return t * t * (3.0f - 2.0f * t);
        }

        private static CEVec3 Lerp(CEVec3 a, CEVec3 b, float t)
        {
            t = Clamp01(t);
            return new CEVec3(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t
            );
        }

        private static CEVec3 LerpAngledPath(CEVec3 from, CEVec3 to, float t01, float firstPortion, float angleDeg, float distance)
        {
            t01 = Clamp01(t01);
            if (firstPortion < 0.05f) firstPortion = 0.05f;
            if (firstPortion > 0.95f) firstPortion = 0.95f;

            float rad = angleDeg * 0.017453292519943295f;
            float dirX = (float)Math.Sin(rad);
            float dirY = (float)Math.Cos(rad);
            CEVec3 knee = new CEVec3(from.x + dirX * distance, from.y + dirY * distance, from.z);

            if (t01 <= firstPortion)
            {
                float aT = Smooth01(t01 / firstPortion);
                return Lerp(from, knee, aT);
            }
            else
            {
                float bT = Smooth01((t01 - firstPortion) / (1.0f - firstPortion));
                return Lerp(knee, to, bT);
            }
        }

        private static CEVec3 FacingAdjusted(CEVec3 v, bool facingRight, bool mirrorXForLeft)
        {
            if (!facingRight && mirrorXForLeft) return new CEVec3(-v.x, v.y, v.z);
            return v;
        }

        public override void Start()
        {
            playerTf = Entity.GetComponent<TransformComponent>();
            playerAnim = Entity.GetComponent<Animator>();
            playerMovement = Entity.GetScript<PlayerMovement>();
            if (playerMovement != null) playerMovement.lockBookGlowToPlayer = false;

            if (string.IsNullOrEmpty(bookGlowEntityName)) return;

            glowEntity = Entity.Find(bookGlowEntityName);
            if (glowEntity == null) return;

            glowTf = glowEntity.GetComponent<TransformComponent>();
        }

        public override void LateUpdate()
        {
            if (playerTf == null || glowEntity == null || glowTf == null) return;

            bool isLightMode = playerMovement != null && playerMovement.IsLightMode;
            bool isSwitching = playerMovement != null && playerMovement.IsSwitching;
            float switchT = playerMovement != null ? playerMovement.SwitchProgress01 : 1.0f;
            bool switchStartLight = playerMovement != null && playerMovement.SwitchStartIsLight;
            bool switchTargetLight = playerMovement != null && playerMovement.SwitchTargetIsLight;

            if (previewEnabled)
            {
                isSwitching = true;
                switchT = Clamp01(previewSwitchT01);
                switchStartLight = previewStartIsLight;
                switchTargetLight = previewTargetIsLight;
                isLightMode = previewTargetIsLight;
            }

            if (onlyGlowInLightMode)
            {
                bool show = isLightMode;
                if (isSwitching)
                {
                    show = switchStartLight || switchTargetLight;
                }
                if (previewEnabled && previewForceVisible) show = true;

                if (lastActiveState != show)
                {
                    InternalCalls.SetActive(glowEntity.ID, show);
                    lastActiveState = show;
                }

                if (!show) return;
            }
            else
            {
                if (!lastActiveState) {
                    InternalCalls.SetActive(glowEntity.ID, true);
                    lastActiveState = true;
                }
            }

            bool facingRight = previewEnabled ? previewFacingRight : (playerMovement == null || playerMovement.facingRight);

            string clip = playerAnim != null ? playerAnim.CurrentClip : "";
            bool isSwitchClip = !string.IsNullOrEmpty(clip) && clip.IndexOf("Switch", StringComparison.OrdinalIgnoreCase) >= 0;
            if (previewEnabled) isSwitchClip = previewUseSwitchClip;

            CEVec3 behindOffset = facingRight ? offsetBehindRight : offsetBehindLeft;
            CEVec3 bookOffset = facingRight
                ? (isSwitchClip ? offsetRightSwitch : offsetRight)
                : (isSwitchClip ? offsetLeftSwitch : offsetLeft);

            if (overrideStartEnd)
            {
                behindOffset = FacingAdjusted(startOffset, facingRight, mirrorXForLeft);
                bookOffset = FacingAdjusted(endOffset, facingRight, mirrorXForLeft);
            }

            CEVec3 offset = isLightMode ? bookOffset : behindOffset;
            if (isSwitching)
            {
                if (!switchStartLight && switchTargetLight)
                {
                    float t = Smooth01(Clamp01(switchT / 0.5f));
                    offset = useAngledPath ? LerpAngledPath(behindOffset, bookOffset, t, pathFirstPortion, pathAngleDeg, pathDistance) : Lerp(behindOffset, bookOffset, t);
                }
                else if (switchStartLight && !switchTargetLight)
                {
                    float t = Smooth01(Clamp01((switchT - 0.5f) / 0.5f));
                    offset = useAngledPath ? LerpAngledPath(bookOffset, behindOffset, t, pathFirstPortion, pathAngleDeg, pathDistance) : Lerp(bookOffset, behindOffset, t);
                }
                else
                {
                    offset = switchTargetLight ? bookOffset : behindOffset;
                }
            }

            CEVec3 p = playerTf.Position;
            CEVec3 g = glowTf.Position;
            glowTf.Position = new CEVec3(
                p.x + offset.x,
                p.y + offset.y,
                followZ ? (p.z + offset.z) : g.z
            );
        }
    }
}
