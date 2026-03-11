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
        [SerializeField] public float moveSpeed = 1.0f;
        [SerializeField] public bool useAlphaTransition = true;
        [SerializeField] public float alphaStart = 0.0f;
        [SerializeField] public float alphaEnd = 1.0f;
        [SerializeField] public float alphaSpeed = 1.0f;
        [SerializeField] public bool forceShowStart = false;
        [SerializeField] public bool forceShowEnd = false;
        [SerializeField] public bool showOffsetGizmos = true;
        [SerializeField] public bool useDragHandleEntities = false;
        [SerializeField] public string startHandleEntityName = "BookGlow_StartHandle";
        [SerializeField] public string endHandleEntityName = "BookGlow_EndHandle";
        [SerializeField] public string lineEntityName = "BookGlow_PathLine";
        [SerializeField] public float handleRadius = 14.0f;
        [SerializeField] public float lineThickness = 4.0f;
        [SerializeField] public bool finalizeAndDisableHandles = false;
        [SerializeField] public bool hideHandleEntitiesWhenDisabled = true;
        [SerializeField] public bool snapStartToEnd = false;
        [SerializeField] public bool snapEndToStart = false;

        [SerializeField] public bool overrideStartEnd = false;
        [SerializeField] public bool mirrorXForLeft = true;
        [SerializeField] public bool lockEndToBookCenter = true;
        [SerializeField] public bool useAirEndOffset = false;
        [SerializeField] public CEVec3 startOffset = new CEVec3(-30.0f, -5.0f, 0.0f);
        [SerializeField] public CEVec3 endOffset = new CEVec3(30.0f, 0.0f, 0.0f);
        [SerializeField] public CEVec3 endOffsetAir = new CEVec3(30.0f, 0.0f, 0.0f);
        [SerializeField] public bool snapAirToGround = false;
        [SerializeField] public bool snapGroundToAir = false;

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
        private LightComponent glowLight;
        private bool lastActiveState = true;
        private Entity startHandleEntity;
        private Entity endHandleEntity;
        private Entity lineEntity;
        private TransformComponent startHandleTf;
        private TransformComponent endHandleTf;
        private TransformComponent lineTf;
        private ShapeComponent startHandleShape;
        private ShapeComponent endHandleShape;
        private ShapeComponent lineShape;
        private CEVec3 lastStartHandlePos;
        private CEVec3 lastEndHandlePos;
        private bool handleCacheValid = false;

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

        private CEVec3 GetEndOffsetCanonical()
        {
            if (useAirEndOffset && playerMovement != null && !playerMovement.IsGrounded) return endOffsetAir;
            return endOffset;
        }

        private void SetEndOffsetCanonical(CEVec3 v)
        {
            if (useAirEndOffset && playerMovement != null && !playerMovement.IsGrounded) endOffsetAir = v;
            else endOffset = v;
        }

        private float GetMirrorSign(bool facingRight)
        {
            if (!mirrorXForLeft) return 1.0f;
            if (playerTf != null) return SignNonZero(playerTf.Size.x);
            return facingRight ? 1.0f : -1.0f;
        }

        private void ComputeOffsets(bool facingRight, bool isSwitchClip, out CEVec3 behindOffset, out CEVec3 bookOffset)
        {
            behindOffset = facingRight ? offsetBehindRight : offsetBehindLeft;
            bookOffset = facingRight
                ? (isSwitchClip ? offsetRightSwitch : offsetRight)
                : (isSwitchClip ? offsetLeftSwitch : offsetLeft);

            if (lockEndToBookCenter)
            {
                float sx = GetMirrorSign(facingRight);
                CEVec3 e = GetEndOffsetCanonical();
                bookOffset = new CEVec3(e.x * sx, e.y, e.z);
            }

            if (overrideStartEnd)
            {
                float sx = GetMirrorSign(facingRight);
                behindOffset = new CEVec3(startOffset.x * sx, startOffset.y, startOffset.z);
                CEVec3 e = GetEndOffsetCanonical();
                bookOffset = new CEVec3(e.x * sx, e.y, e.z);
            }
        }

        private static float SignNonZero(float v)
        {
            return v < 0.0f ? -1.0f : 1.0f;
        }

        private static float AbsNonZero(float v)
        {
            float a = Math.Abs(v);
            return a < 0.0001f ? 1.0f : a;
        }

        private void SetGlowLocalPosFromWorldOffset(CEVec3 worldOffset, CEVec3 parentWorldPos, CEVec2 parentSize, float childWorldZ)
        {
            if (!InternalCalls.EntityHasParent(glowEntity.ID)) return;

            float signX = SignNonZero(parentSize.x);
            float denomX = AbsNonZero(parentSize.x);
            float denomY = AbsNonZero(parentSize.y);

            CEVec3 local = new CEVec3(
                (worldOffset.x * signX) / denomX,
                worldOffset.y / denomY,
                followZ ? worldOffset.z : (childWorldZ - parentWorldPos.z)
            );

            InternalCalls.HierarchySetLocalPos(glowEntity.ID, ref local);
        }

        private static bool NearlyEqual(CEVec3 a, CEVec3 b, float eps)
        {
            return Math.Abs(a.x - b.x) <= eps && Math.Abs(a.y - b.y) <= eps && Math.Abs(a.z - b.z) <= eps;
        }

        private CEVec3 ToCanonicalOffset(CEVec3 handleWorldPos, CEVec3 playerWorldPos, bool facingRight)
        {
            CEVec3 d = handleWorldPos - playerWorldPos;
            if (!facingRight && mirrorXForLeft) d.x = -d.x;
            return d;
        }

        private void TryBindHandleEntities()
        {
            if (!useDragHandleEntities) return;
            if (handleCacheValid) return;

            if (!string.IsNullOrEmpty(startHandleEntityName)) startHandleEntity = Entity.Find(startHandleEntityName);
            if (!string.IsNullOrEmpty(endHandleEntityName)) endHandleEntity = Entity.Find(endHandleEntityName);
            if (!string.IsNullOrEmpty(lineEntityName)) lineEntity = Entity.Find(lineEntityName);

            if (startHandleEntity != null) startHandleTf = startHandleEntity.GetComponent<TransformComponent>();
            if (endHandleEntity != null) endHandleTf = endHandleEntity.GetComponent<TransformComponent>();
            if (lineEntity != null) lineTf = lineEntity.GetComponent<TransformComponent>();

            if (startHandleEntity != null)
            {
                if (startHandleEntity.GetComponent<ShapeComponent>() == null) startHandleEntity.Add<ShapeComponent>();
                startHandleShape = startHandleEntity.GetComponent<ShapeComponent>();
                if (startHandleShape != null)
                {
                    startHandleShape.Type = ShapeType.Circle;
                    startHandleShape.Color = new CEColor(0.2f, 0.9f, 0.2f, 1.0f);
                }
                if (startHandleTf != null) startHandleTf.Size = new CEVec2(handleRadius * 2.0f, handleRadius * 2.0f);
            }

            if (endHandleEntity != null)
            {
                if (endHandleEntity.GetComponent<ShapeComponent>() == null) endHandleEntity.Add<ShapeComponent>();
                endHandleShape = endHandleEntity.GetComponent<ShapeComponent>();
                if (endHandleShape != null)
                {
                    endHandleShape.Type = ShapeType.Circle;
                    endHandleShape.Color = new CEColor(0.2f, 0.6f, 1.0f, 1.0f);
                }
                if (endHandleTf != null) endHandleTf.Size = new CEVec2(handleRadius * 2.0f, handleRadius * 2.0f);
            }

            if (lineEntity != null)
            {
                if (lineEntity.GetComponent<ShapeComponent>() == null) lineEntity.Add<ShapeComponent>();
                lineShape = lineEntity.GetComponent<ShapeComponent>();
                if (lineShape != null)
                {
                    lineShape.Type = ShapeType.Line;
                    lineShape.Color = new CEColor(1.0f, 1.0f, 0.2f, 1.0f);
                    lineShape.Thickness = lineThickness;
                }
            }

            if (startHandleTf != null) lastStartHandlePos = startHandleTf.Position;
            if (endHandleTf != null) lastEndHandlePos = endHandleTf.Position;

            handleCacheValid = startHandleTf != null || endHandleTf != null || lineShape != null;
        }

        private void DisableHandleEntities()
        {
            if (startHandleEntity == null && !string.IsNullOrEmpty(startHandleEntityName)) startHandleEntity = Entity.Find(startHandleEntityName);
            if (endHandleEntity == null && !string.IsNullOrEmpty(endHandleEntityName)) endHandleEntity = Entity.Find(endHandleEntityName);
            if (lineEntity == null && !string.IsNullOrEmpty(lineEntityName)) lineEntity = Entity.Find(lineEntityName);

            if (startHandleEntity != null)
            {
                ShapeComponent s = startHandleEntity.GetComponent<ShapeComponent>();
                if (s != null) s.Alpha = 0.0f;
                if (hideHandleEntitiesWhenDisabled) InternalCalls.SetActive(startHandleEntity.ID, false);
            }
            if (endHandleEntity != null)
            {
                ShapeComponent s = endHandleEntity.GetComponent<ShapeComponent>();
                if (s != null) s.Alpha = 0.0f;
                if (hideHandleEntitiesWhenDisabled) InternalCalls.SetActive(endHandleEntity.ID, false);
            }
            if (lineEntity != null)
            {
                ShapeComponent s = lineEntity.GetComponent<ShapeComponent>();
                if (s != null) s.Alpha = 0.0f;
                if (hideHandleEntitiesWhenDisabled) InternalCalls.SetActive(lineEntity.ID, false);
            }
        }

        private static void DrawCircleThick(CEVec3 center, float radius, int segments, float thickness, CEVec3 color)
        {
            if (segments < 8) segments = 8;
            if (segments > 64) segments = 64;
            if (radius <= 0.001f) return;

            float step = 6.28318530718f / segments;
            CEVec3 prev = new CEVec3(center.x + radius, center.y, center.z);
            for (int i = 1; i <= segments; ++i)
            {
                float a = i * step;
                CEVec3 next = new CEVec3(
                    center.x + radius * (float)Math.Cos(a),
                    center.y + radius * (float)Math.Sin(a),
                    center.z
                );
                Debug.DrawLine(ref prev, ref next, thickness, ref color);
                prev = next;
            }
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
            glowLight = glowEntity.GetComponent<LightComponent>();
            lastActiveState = InternalCalls.GetActive(glowEntity.ID);
        }

        public override void LateUpdate()
        {
            if (playerTf == null || glowEntity == null || glowTf == null) return;

            if (finalizeAndDisableHandles)
            {
                finalizeAndDisableHandles = false;
                useDragHandleEntities = false;
                showOffsetGizmos = false;
                forceShowStart = false;
                forceShowEnd = false;

                DisableHandleEntities();
                handleCacheValid = false;
            }

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

            if (snapStartToEnd)
            {
                startOffset = GetEndOffsetCanonical();
                snapStartToEnd = false;
            }
            if (snapEndToStart)
            {
                SetEndOffsetCanonical(startOffset);
                snapEndToStart = false;
            }
            if (snapAirToGround)
            {
                endOffsetAir = endOffset;
                snapAirToGround = false;
            }
            if (snapGroundToAir)
            {
                endOffset = endOffsetAir;
                snapGroundToAir = false;
            }

            TryBindHandleEntities();
            if (useDragHandleEntities) overrideStartEnd = true;

            ComputeOffsets(facingRight, isSwitchClip, out CEVec3 behindOffset, out CEVec3 bookOffset);

            CEVec3 offset = isLightMode ? bookOffset : behindOffset;
            float desiredIntensity = isLightMode ? alphaEnd : alphaStart;
            if (forceShowStart) forceShowEnd = false;
            if (forceShowEnd) forceShowStart = false;

            if (forceShowStart)
            {
                offset = behindOffset;
                desiredIntensity = alphaStart;
                isSwitching = false;
            }
            else if (forceShowEnd)
            {
                offset = bookOffset;
                desiredIntensity = alphaEnd;
                isSwitching = false;
            }
            if (isSwitching)
            {
                if (!switchStartLight && switchTargetLight)
                {
                    float tPos = Smooth01(Clamp01((switchT / 0.5f) * moveSpeed));
                    offset = useAngledPath ? LerpAngledPath(behindOffset, bookOffset, tPos, pathFirstPortion, pathAngleDeg, pathDistance) : Lerp(behindOffset, bookOffset, tPos);
                    if (useAlphaTransition)
                    {
                        float tA = Smooth01(Clamp01((switchT / 0.5f) * alphaSpeed));
                        desiredIntensity = alphaStart + (alphaEnd - alphaStart) * tA;
                    }
                }
                else if (switchStartLight && !switchTargetLight)
                {
                    float tPos = Smooth01(Clamp01(((switchT - 0.5f) / 0.5f) * moveSpeed));
                    offset = useAngledPath ? LerpAngledPath(bookOffset, behindOffset, tPos, pathFirstPortion, pathAngleDeg, pathDistance) : Lerp(bookOffset, behindOffset, tPos);
                    if (useAlphaTransition)
                    {
                        float tA = Smooth01(Clamp01(((switchT - 0.5f) / 0.5f) * alphaSpeed));
                        desiredIntensity = alphaEnd + (alphaStart - alphaEnd) * tA;
                    }
                }
                else
                {
                    offset = switchTargetLight ? bookOffset : behindOffset;
                    desiredIntensity = switchTargetLight ? alphaEnd : alphaStart;
                }
            }

            CEVec3 p = playerTf.Position;
            CEVec3 g = glowTf.Position;
            CEVec2 parentSize = playerTf.Size;

            if (useDragHandleEntities && (startHandleTf != null || endHandleTf != null || lineShape != null))
            {
                CEVec3 desiredStartWorld = new CEVec3(p.x + behindOffset.x, p.y + behindOffset.y, followZ ? (p.z + behindOffset.z) : g.z);
                CEVec3 desiredEndWorld = new CEVec3(p.x + bookOffset.x, p.y + bookOffset.y, followZ ? (p.z + bookOffset.z) : g.z);

                bool lmbDown = Input.IsMouseButtonDown(0);
                float eps = 0.0001f;

                if (startHandleTf != null)
                {
                    CEVec3 cur = startHandleTf.Position;
                    bool moved = lmbDown && !NearlyEqual(cur, lastStartHandlePos, eps);
                    if (moved) startOffset = ToCanonicalOffset(cur, p, facingRight);
                    else startHandleTf.Position = desiredStartWorld;
                    lastStartHandlePos = startHandleTf.Position;
                }

                if (endHandleTf != null)
                {
                    CEVec3 cur = endHandleTf.Position;
                    bool moved = lmbDown && !NearlyEqual(cur, lastEndHandlePos, eps);
                    if (moved) SetEndOffsetCanonical(ToCanonicalOffset(cur, p, facingRight));
                    else endHandleTf.Position = desiredEndWorld;
                    lastEndHandlePos = endHandleTf.Position;
                }

                ComputeOffsets(facingRight, isSwitchClip, out behindOffset, out bookOffset);
                desiredStartWorld = new CEVec3(p.x + behindOffset.x, p.y + behindOffset.y, followZ ? (p.z + behindOffset.z) : g.z);
                desiredEndWorld = new CEVec3(p.x + bookOffset.x, p.y + bookOffset.y, followZ ? (p.z + bookOffset.z) : g.z);

                if (lineTf != null) lineTf.Position = new CEVec3(p.x, p.y, followZ ? p.z : g.z);
                if (lineShape != null)
                {
                    lineShape.Type = ShapeType.Line;
                    lineShape.Thickness = lineThickness;
                    lineShape.P1 = behindOffset;
                    lineShape.P2 = bookOffset;
                }
            }

            SetGlowLocalPosFromWorldOffset(offset, p, parentSize, g.z);

            glowTf.Position = new CEVec3(
                p.x + offset.x,
                p.y + offset.y,
                followZ ? (p.z + offset.z) : g.z
            );

            if (glowLight != null)
            {
                glowLight.Intensity = desiredIntensity;
            }
        }

        public override void OnDrawGizmos()
        {
            if (!showOffsetGizmos) return;
            if (playerTf == null) playerTf = Entity.GetComponent<TransformComponent>();
            if (playerTf == null) return;

            bool facingRight = previewEnabled ? previewFacingRight : (playerMovement == null || playerMovement.facingRight);

            string clip = playerAnim != null ? playerAnim.CurrentClip : "";
            bool isSwitchClip = !string.IsNullOrEmpty(clip) && clip.IndexOf("Switch", StringComparison.OrdinalIgnoreCase) >= 0;
            if (previewEnabled) isSwitchClip = previewUseSwitchClip;

            ComputeOffsets(facingRight, isSwitchClip, out CEVec3 behindOffset, out CEVec3 bookOffset);

            CEVec3 p = playerTf.Position;
            CEVec3 behindWorld = new CEVec3(p.x + behindOffset.x, p.y + behindOffset.y, p.z + behindOffset.z);
            CEVec3 bookWorld = new CEVec3(p.x + bookOffset.x, p.y + bookOffset.y, p.z + bookOffset.z);

            float markerRadius = 10.0f;
            float markerThickness = 4.0f;
            CEVec3 behindColor = new CEVec3(0.2f, 0.9f, 0.2f);
            CEVec3 bookColor = new CEVec3(0.2f, 0.6f, 1.0f);
            CEVec3 pathColor = new CEVec3(1.0f, 1.0f, 0.2f);

            DrawCircleThick(behindWorld, markerRadius, 24, markerThickness, behindColor);
            DrawCircleThick(bookWorld, markerRadius, 24, markerThickness, bookColor);

            CEVec3 a = behindWorld;
            CEVec3 b = bookWorld;

            if (useAngledPath)
            {
                float rad = pathAngleDeg * 0.017453292519943295f;
                float dirX = (float)Math.Sin(rad);
                float dirY = (float)Math.Cos(rad);
                CEVec3 kneeOffset = new CEVec3(behindOffset.x + dirX * pathDistance, behindOffset.y + dirY * pathDistance, behindOffset.z);
                CEVec3 kneeWorld = new CEVec3(p.x + kneeOffset.x, p.y + kneeOffset.y, p.z + kneeOffset.z);

                Debug.DrawLine(ref a, ref kneeWorld, 3.0f, ref pathColor);
                Debug.DrawLine(ref kneeWorld, ref b, 3.0f, ref pathColor);
            }
            else
            {
                Debug.DrawLine(ref a, ref b, 3.0f, ref pathColor);
            }
        }
    }
}
