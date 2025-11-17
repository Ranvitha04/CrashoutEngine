using System;

namespace CrashoutsEngine
{
    public class CollisionManifold
    {
        public CEVec3 normal;
        public CEVec3[] contactPoints = new CEVec3[2];
        public float depth;
        public bool collisionOccured;
        public int numOfContacts;
    }
}
