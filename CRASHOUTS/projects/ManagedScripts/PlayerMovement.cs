using System;
namespace CrashoutsEngine
{
    public class PlayerMovement : ScriptBase
    {
        //float horizontalInput;
        //float moveSpeed = 6f;
        //bool isFacingRight = true;
        public float jumpPower = 20000f;
        public static bool isJumping = false;
        public static bool isFalling = false;
        private Animator animator;
        private bool isFacingRight = true;
        //private float lastY;
        //public float FallingThreshold = -0.01f;
        //private int onPlatform = 0;
        //CEVec2 startPos;
        //SpriteRenderer spriteRenderer;

        //[SerializeField] private SwitchCharacter switchcharacter;
        //[SerializeField] private BoxCollider2D playerCollider;

        //public Animator animator;
        public bool IsOnPlatform;
        //public Rigidbody2D platformRb;

        //PressurePlate pressureplate;

        //Rigidbody2D rb;
        //public GameObject GObackground;


        //public void Awake()
        //{
        //    spriteRenderer = GetComponent<SpriteRenderer>();
        //}

        // Start is called before the first frame update
        public override void Start()
        {
            animator = Entity.GetComponent<Animator>();
            if (animator == null)
            {
                Entity.Add<Animator>();
                animator = Entity.GetComponent<Animator>();
            }
            if (animator != null)
            {
                animator.SetBool("IsRunning", false);
                animator.SetBool("IsFalling", false);
                animator.SetBool("IsJumping", false);
            }

            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, OnCollideExit);
        }

        // Update is called once per frame
        public override void Update()
        {
            var rb = Entity.GetComponent<RigidBodyComponent>();
            var tf = Entity.GetComponent<TransformComponent>();

            var movingLeft = Input.IsKeyHeld(Key.A);
            var movingRight = Input.IsKeyHeld(Key.D);
            var moving = movingLeft || movingRight;

            if (movingLeft)
            {
                rb.AddForce(new CEVec3(-500f, 0f, 0f));
                isFacingRight = false;
                animator?.SetFlip(true);
            }

            if (movingRight)
            {
                rb.AddForce(new CEVec3(500f, 0f, 0f));
                isFacingRight = true;
                animator?.SetFlip(false);
            }

            if (animator != null)
            {
                animator.SetBool("IsRunning", moving);
            }

            if (Input.IsKeyTriggered(Key.Space) && !isJumping)
            {
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                isJumping = true;
                isFalling = false;
                animator?.SetBool("IsJumping", true);
                animator?.SetBool("IsFalling", false);
            }

            if (!moving && animator != null && !isJumping && !isFalling)
            {
                animator.SetBool("IsRunning", false);
            }
        }

        private void OnCollideEnter(uint other)
        {
            isJumping = false;
            isFalling = false;
            animator?.SetBool("IsJumping", false);
            animator?.SetBool("IsFalling", false);
        }

        private void OnCollideExit(uint other)
        {
            if (!isJumping)
            {
                isFalling = true;
                animator?.SetBool("IsFalling", true);
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
