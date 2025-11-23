using System;
namespace CrashoutsEngine
{
    public class PlayerMovement : ScriptBase
    {
        float horizontalInput;
        //float moveSpeed = 6f;
        bool isFacingRight = true;
        bool moveLeft;
        bool moveRight;
        bool isMoving = false;
        bool wasMoving = false;
        public float jumpPower = 20000f;
        public static bool isJumping = false;
        public static bool isFalling = false;
        //private float lastY;
        //public float FallingThreshold = -0.01f;
        //private int onPlatform = 0;
        //CEVec2 startPos;
        //SpriteRenderer spriteRenderer;

        //[SerializeField] private SwitchCharacter switchcharacter;
        //[SerializeField] private BoxCollider2D playerCollider;

        private Animator animator;
        public bool IsOnPlatform;
        //public Rigidbody2D platformRb;

        //PressurePlate pressureplate;

        //Rigidbody2D rb;
        RigidBodyComponent rb;

        //public GameObject GObackground;


        //public void Awake()
        //{
        //    spriteRenderer = GetComponent<SpriteRenderer>();
        //}

        // Start is called before the first frame update
        public override void Start()
        {
            animator = Entity.GetComponent<Animator>();
            rb = Entity.GetComponent<RigidBodyComponent>();
            //lastY = transform.position.y;
            animator.SetBool("IsRunning", false);
            animator.SetBool("IsFalling", false);
            animator.SetBool("IsJumping", false);
            animator.SetBool("PressQ", false);
            //startPos = transform.position;

            //GObackground.SetActive(false);

            //AudioManager.GetInstance().Play("ambient");

        }

        // Update is called once per frame
        public override void Update()
        {
            var tf = Entity.GetComponent<TransformComponent>();
            //horizontalInput = Input.GetAxis("Horizontal");
            horizontalInput = 0f;
            moveLeft = Input.IsKeyPressed(Key.A);
            moveRight = Input.IsKeyPressed(Key.D);
            if (moveLeft)
            {
                horizontalInput = -1f;
            }
            else if (moveRight)
            {
                horizontalInput = 1f;
            }
            //lastY = transform.position.y;
            isMoving = moveLeft || moveRight;
            if (isMoving && !wasMoving)
            {
                //Console.WriteLine("[C#] Player started moving -> Play footsteps");
                AudioManager.GetInstance().Play("footsteps");
            }
            else if (!isMoving && wasMoving)
            {
                //Console.WriteLine("[C#] Player stopped moving -> Stop footsteps_loop");
                AudioManager.GetInstance().Stop("footsteps");
            }

            FlipSprite(horizontalInput);

            //if (Input.GetButtonDown("Jump") && !isJumping)
            //{
            //    rb.linearVelocity = new Vector2(rb.linearVelocity.x, jumpPower);
            //    isJumping = true;
            //    animator.SetBool("IsJumping", true);

            //    switchcharacter?.SwitchAvatar();
            //}

            if (Input.IsKeyTriggered(Key.Space) && !isJumping)
            {
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                isJumping = true;
                animator.SetBool("IsJumping", true);
            }
            //if (animator.GetBool("IsJumping") && rb.linearVelocity.y < 0 && IsOnPlatform == false)
            //{
            //    // isFalling = true;
            //    animator.SetBool("IsFalling", true);
            //    animator.SetBool("IsJumping", false);
            //    isJumping = false;
            //    switchcharacter?.SwitchAvatar();
            //}
            /*  else if (rb.linearVelocity.y < 0)
              {
                  Debug.Log("Keepsfalling");
                  animator.SetBool("IsFalling", true);
                  switchcharacter?.SwitchAvatar();
              } */
            var vel = rb.LinearVelocity;

            // Jump started and peak reached then beginning to fall
            if (animator.GetBool("IsJumping") && vel.y < 0f && !IsOnPlatform)
            {
                animator.SetBool("IsFalling", true);
                animator.SetBool("IsJumping", false);

                isJumping = false;
                isFalling = true;
            }

            // Player is falling normally
            else if (vel.y < 0f && !IsOnPlatform)
            {
                animator.SetBool("IsFalling", true);
                isFalling = true;
            }

            // Player lands on platform so reset falling/jumping??
            else if (IsOnPlatform && vel.y == 0f)
            {
                animator.SetBool("IsFalling", false);
                animator.SetBool("IsJumping", false);

                isJumping = false;
                isFalling = false;
            }
        }

        //private void FixedUpdate()
        //{
        //    if (IsOnPlatform)
        //    {
        //        rb.linearVelocity = new Vector2((horizontalInput * moveSpeed) + platformRb.linearVelocity.x, rb.linearVelocity.y);
        //    }

        //    else
        //    {
        //        rb.linearVelocity = new Vector2(horizontalInput * moveSpeed, rb.linearVelocity.y);
        //    }
        //    if (horizontalInput != 0)
        //    {
        //        animator.SetBool("IsRunning", true);
        //    }
        //    else if (!animator.GetBool("IsJumping") && !animator.GetBool("IsFalling") && !animator.GetBool("PressQ"))
        //    {
        //        animator.SetBool("IsRunning", false);
        //        if (animator.GetBool("IsLight"))
        //        {
        //            animator.Play("Idle Light");
        //        }
        //        else
        //        {
        //            // Debug.Log("Play Idle");
        //            animator.Play("Idle");
        //        }
        //    }
        //}
        public override void FixedUpdate()
        {
            // movement
            if (horizontalInput != 0f)
            {
                // force multiplier
                float force = 500f * horizontalInput;

                rb.AddForce(new CEVec3(force, 0f, 0f));
            }

            // animation
            if (horizontalInput != 0f)
            {
                animator.SetBool("IsRunning", true);
            }
            else
            {
                animator.SetBool("IsRunning", false);

                if (animator.GetBool("IsLight"))
                {
                    animator.Play("Idle Light");
                }
                else
                {
                    animator.Play("Idle");
                }
            }
        }

        //void FlipSprite()
        //{
        //    if (isFacingRight && horizontalInput < 0f || !isFacingRight && horizontalInput > 0f)
        //    {
        //        isFacingRight = !isFacingRight;
        //        Vector3 ls = transform.localScale;
        //        ls.x *= -1f;
        //        transform.localScale = ls;
        //    }
        //}

        void FlipSprite(float horizontalInput)
        {
            if (horizontalInput == 0f)
                return;

            // Same logic as Unity
            if ((isFacingRight && horizontalInput < 0f) || (!isFacingRight && horizontalInput > 0f))
            {
                isFacingRight = !isFacingRight;

                // Flip horizontally using animator
                animator.SetFlip(!isFacingRight);
            }
        }


        //private void OnCollisionEnter2D(Collision2D collision)
        //{
        //    Debug.Log("Collided enter =>" + collision.gameObject.name);
        //    isJumping = false;
        //   // isFalling = false;
        //    animator.SetBool("IsJumping", false);
        //    animator.SetBool("IsFalling", false);
        //    // switchcharacter.SwitchAvatar();
        //    ++onPlatform;

        //    if (collision.gameObject.CompareTag("Enemy"))
        //    {
        //        Die();
        //    }
        //    if (animator.GetBool("IsLight"))
        //    {
        //        return;
        //    }
        //    else
        //    {
        //        if (collision.gameObject.CompareTag("Lava"))
        //        {
        //            Die();
        //        }
        //    }
        //}

        //private void OnCollisionExit2D(Collision2D collision)
        //{
        //    --onPlatform;
        //    if (!animator.GetBool("IsJumping") && onPlatform == 0 && IsOnPlatform == false)
        //    {
        //            Debug.Log("Collided exit =>" + collision.gameObject.name);
        //            animator.SetBool("IsFalling", true);
        //            switchcharacter?.SwitchAvatar();
        //    }
        //}

        //void Die()
        //{
        //    GObackground.SetActive(true);

        //    //StartCoroutine(Respawn(0.5f));
        //    rb.linearVelocity = new Vector2(0, 0);
        //    spriteRenderer.enabled = false; //disappears when collide with enemy
        // }

        //public void respawn()
        //{
        //    transform.position = startPos;
        //    spriteRenderer.enabled = true; //appears again after cooldown

        //    //animator.Play("Idle");
        //    //Debug.Log("respawn idle here"); //gets to this line but still plays falling animation? does it need yo be in collisionExit
        //    GObackground.SetActive(false);

        //}
    }
}
