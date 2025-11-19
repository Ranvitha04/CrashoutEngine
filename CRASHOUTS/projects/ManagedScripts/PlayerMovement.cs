using System;
using System.Numerics;
namespace CrashoutsEngine
{
    public class PlayerMovement : ScriptBase
    {
        //float horizontalInput;
        //float moveSpeed = 6f;
        bool isFacingRight = true;
        public float jumpPower = 20000f;
        public static bool isJumping = false;
        public static bool isFalling = false;
        private float lastY;
        public float FallingThreshold = -0.01f;
        private int onPlatform = 0;
        CEVec2 startPos;
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
        void Start()
        {
            //rb = GetComponent<Rigidbody2D>();
            //lastY = transform.position.y;
            //animator.SetBool("IsRunning", false);
            //animator.SetBool("IsFalling", false);
            //animator.SetBool("IsJumping", false);
            //animator.SetBool("PressQ", false);
            //startPos = transform.position;

            //GObackground.SetActive(false);

            //AudioManager.GetInstance().Play("ambient");

        }

        // Update is called once per frame
        void Update()
        {

            var rb = Entity.GetComponent<RigidBodyComponent>();
            var tf = Entity.GetComponent<TransformComponent>();
            //horizontalInput = Input.GetAxis("Horizontal");
            if (Input.IsKeyHeld(Key.A))
            {
                rb.AddForce(new CEVec3(-500f, 0f, 0f));
            }

            if (Input.IsKeyHeld(Key.D))
            {
                rb.AddForce(new CEVec3(500f, 0f, 0f));
            }




            //isnt it divide by delta time to get per second? XD - rain
            //float distancePerSecondSinceLastFrame = (transform.position.y - lastY) * Time.deltaTime;
            //lastY = transform.position.y;


            //FlipSprite();

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
                //animator.SetBool("IsJumping", true);
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
