using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
	// hopefully this will work -Rain
	// Wrapper around native CollisionEventManager callbacks
	public static class CollisionEvents
	{
		// C# delegate that matches: void (*)(Entity)
		public delegate void CollisionCallback(uint otherEntity);

		// COLLIDE (solid colliders)

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterCollideEnter(uint entity, CollisionCallback callback);

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterCollideStay(uint entity, CollisionCallback callback);

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterCollideExit(uint entity, CollisionCallback callback);


		// TRIGGER (trigger colliders)

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterTriggerEnter(uint entity, CollisionCallback callback);

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterTriggerStay(uint entity, CollisionCallback callback);

		[MethodImpl(MethodImplOptions.InternalCall)]
		public static extern void RegisterTriggerExit(uint entity, CollisionCallback callback);
	}
}
