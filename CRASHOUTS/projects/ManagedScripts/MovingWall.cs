using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MovingWall : ScriptBase
    {
        public int PlatesActive = 0;
        [SerializeField]
        int PlatesRequired = 1; // Set this to 2 if you need 2 plates, etc.
        [SerializeField]
        bool MoveRight = false;
        [SerializeField]
        float MoveDist = 10.0f;
        [SerializeField]
        float MoveSpeed = 5.0f;

        private bool _isOpen = false;
        private bool _stopMoving = false;
        //private RigidBodyComponent _rb;
        TransformComponent transform;
        private CEVec2 oldXY, newXY;

        void OpenWall()
        {
            Console.WriteLine("Wall Opening!");
            _isOpen = true;
            // Move the wall up
            // _rb.LinearVelocity = new CEVec3(0, 5, 0); 
        }

        void CloseWall()
        {
            Console.WriteLine("Wall Closing!");
            _isOpen = false;
            // Move the wall down
            // _rb.LinearVelocity = new CEVec3(0, -5, 0);
        }

        public override void Start()
        {
            _isOpen = false;
            _stopMoving = false;
            transform = Entity.GetComponent<TransformComponent>();
            //_rb = Entity.GetComponent<RigidBodyComponent>();
            oldXY = new CEVec2(transform.Position.x, transform.Position.y);
            if (MoveRight)
            {
                newXY = new CEVec2(transform.Position.x + MoveDist, transform.Position.y);
            }
            else
            {
                newXY = new CEVec2(transform.Position.x, transform.Position.y + MoveDist);
            }
        }

        public override void Update()
        {
            // Logic: If enough plates are pressed, Open.
            if (PlatesActive >= PlatesRequired && !_isOpen)
            {
                OpenWall();
            }
            else if (PlatesActive < PlatesRequired && _isOpen)
            {
                CloseWall();
            }
            
                //Console.WriteLine(Time.DT);
            if (_isOpen && !_stopMoving)
            {
                //Console.WriteLine(Time.DT);
                float increment = MoveSpeed * ((float)Time.DT);
                //Console.WriteLine(increment);
                // Force transform location now because collider kinda wonky (NEED FIXING)
                if (transform != null)
                {
                    if (MoveRight)
                    {
                        oldXY = new CEVec2(transform.Position.x + increment, transform.Position.y);
                        if (oldXY.x >= newXY.x) _stopMoving = true;
                    }
                    else
                    {
                        oldXY = new CEVec2(transform.Position.x, transform.Position.y + increment);
                        if (oldXY.y >= newXY.y) _stopMoving = true;
                    }
                    transform.Position = new CEVec3(oldXY.x, oldXY.y, transform.Position.z);
                    //Debug.Log("move");
                }
                //Debug.Log("wtdf");
            }
        }

        public override void FixedUpdate()
        {
            //if (_isOpen && !_stopMoving)
            //{
            //}
        }
    }
}