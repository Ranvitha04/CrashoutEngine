/*!
@file       collisionmanifold.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      class for collision manifold representation

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
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
