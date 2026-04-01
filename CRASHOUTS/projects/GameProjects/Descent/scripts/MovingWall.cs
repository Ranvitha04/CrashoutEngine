/*!
@file       MovingWall.cs
@author     Renfred Lee
@author     Shahrin Idzuan
@date       27/11/2025
@brief      Moving wall that opens when pressure plates are activated.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MovingWall : ScriptBase
    {
        public int PlatesActive = 0;
        [SerializeField]
        private int PlatesRequired = 1; // Set this to 2 if you need 2 plates, etc.
        [SerializeField]
        private bool MoveRight = false;
        [SerializeField]
        private float MoveDist = 10.0f; // Positive for Right/Up, Negative for Left/Down
        [SerializeField]
        private float MoveSpeed = 5.0f;

        [SerializeField]
        private bool emitMoveDust = true;
        [SerializeField]
        private string landingDustEntityName = "";

        private bool _isOpen = false;
        private bool _stopMoving = false;
        private bool _dustPending = false;
        private uint _landingDustID = 0;
        TransformComponent transform;
        private CEVec2 newXY, startXY;

        private void ResolveDustEntity()
        {
            if (!emitMoveDust) return;
            if (_landingDustID != 0) return;

            string resolved = landingDustEntityName;
            if (!string.IsNullOrEmpty(resolved))
            {
                _landingDustID = InternalCalls.FindEntityByName(resolved);
            }

            if (_landingDustID == 0)
            {
                _landingDustID = InternalCalls.FindEntityByName("PlatformLandingDust_" + Entity.ID.ToString());
            }

            if (_landingDustID == 0)
            {
                _landingDustID = InternalCalls.FindEntityByName("PlatformLandingDust");
            }
        }

        void OpenWall()
        {
            Console.WriteLine("Wall Opening!");
            _isOpen = true;
            _stopMoving = false;
            _dustPending = true;
        }

        void CloseWall()
        {
            Console.WriteLine("Wall Closing!");
            _isOpen = false;
            _stopMoving = false;
            _dustPending = true;
        }

        public override void Start()
        {
            AudioManager.SetClip3DDistances(Entity.ID, "rockSFX", 20.0f, 1600.0f);

            _isOpen = false;
            _stopMoving = false;
            transform = Entity.GetComponent<TransformComponent>();
            if (transform != null)
            {
                startXY = new CEVec2(transform.Position.x, transform.Position.y);
                if (MoveRight)
                {
                    newXY = new CEVec2(transform.Position.x + MoveDist, transform.Position.y);
                }
                else
                {
                    newXY = new CEVec2(transform.Position.x, transform.Position.y + MoveDist);
                }
            }

            ResolveDustEntity();
        }

        public override void Update()
        {
            // Logic: If enough plates are pressed, Open.
            if (PlatesActive >= PlatesRequired && !_isOpen)
            {
                AudioManager.GetInstance().PlayAttached("rockSFX", Entity.ID);
                OpenWall();
            }
            else if (PlatesActive < PlatesRequired && _isOpen)
            {
                AudioManager.GetInstance().PlayAttached("rockSFX", Entity.ID);
                CloseWall();
            }
            
            if (!_stopMoving && transform != null)
            {
                float step = MoveSpeed * (float)Time.DT;
                CEVec2 target = _isOpen ? newXY : startXY;
                CEVec3 pos = transform.Position;
                CEVec2 current = new CEVec2(pos.x, pos.y);

                if (MoveRight)
                {
                    float diff = target.x - current.x;
                    float direction = Math.Sign(diff);
                    if (direction == 0)
                    {
                        _stopMoving = true;
                    }
                    else
                    {
                        float nextX = current.x + direction * step;
                        // Clamp and Stop check
                        if ((direction > 0 && nextX >= target.x) || (direction < 0 && nextX <= target.x))
                        {
                            nextX = target.x;
                            _stopMoving = true;
                        }
                        transform.Position = new CEVec3(nextX, current.y, pos.z);
                    }
                }
                else
                {
                    float diff = target.y - current.y;
                    float direction = Math.Sign(diff);
                    if (direction == 0)
                    {
                        _stopMoving = true;
                    }
                    else
                    {
                        float nextY = current.y + direction * step;
                        // Clamp and Stop check
                        if ((direction > 0 && nextY >= target.y) || (direction < 0 && nextY <= target.y))
                        {
                            nextY = target.y;
                            _stopMoving = true;
                        }
                        transform.Position = new CEVec3(current.x, nextY, pos.z);
                    }
                }
            }

            if (_dustPending && _stopMoving)
            {
                TriggerDoorDust();
                _dustPending = false;
            }
        }

        private void TriggerDoorDust()
        {
            if (!emitMoveDust) return;
            if (_landingDustID == 0) ResolveDustEntity();
            if (_landingDustID == 0 || transform == null) return;

            var dustTf = new Entity(_landingDustID).GetComponent<TransformComponent>();
            if (dustTf != null) {
                if (InternalCalls.EntityHasParent(_landingDustID))
                {
                    CEVec3 local = new CEVec3(0.0f, -Math.Abs(transform.Size.y) * 0.5f, 0.0f);
                    InternalCalls.HierarchySetLocalPos(_landingDustID, ref local);
                }
                else
                {
                    CEVec3 pos = transform.Position;
                    pos.y -= Math.Abs(transform.Size.y) * 0.5f;
                    dustTf.Position = new CEVec3(pos.x, pos.y, dustTf.Position.z);
                }
            }

            InternalCalls.Particle_TriggerBurst(_landingDustID);
        }

        public override void FixedUpdate()
        {
            //if (_isOpen && !_stopMoving)
            //{
            //}
        }
    }
}

