/*!
@file       LoopingAnimation.cs
@author     mohamadamirakmal.b
@date       30/03/2026
@brief      A simple script to play a specified animation clip in a loop.
*/

using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class LoopingAnimation : ScriptBase
    {
        [SerializeField] private string animationName = "";
        private Animator _animator;

        public override void Start()
        {
            _animator = Entity.GetComponent<Animator>();
            if (_animator != null && !string.IsNullOrEmpty(animationName))
            {
                _animator.Play(animationName, true);
            }
        }

        public override void Update()
        {
            if (_animator != null && !string.IsNullOrEmpty(animationName))
            {
                // Safety check: if the animation stops for some reason, restart it.
                // (Though engine-side KFWrap::Loop usually handles this).
                if (!_animator.IsPlaying)
                {
                    _animator.Play(animationName, true);
                }
            }
        }
    }
}