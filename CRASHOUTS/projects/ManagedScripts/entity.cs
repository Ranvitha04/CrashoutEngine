using System;
using System.Numerics;

namespace CrashoutsEngine
{
    public class Entity
    {
        public uint ID { get; private set; }

        public Entity(uint id)
        {
            ID = id;
        }

        // Check if this entity has a Transform component
        public bool HasTransform => InternalCalls.EntityHasTransform(ID);
    
        // Lazy-load a Transform object for this entity
        private Transform _transform;
        public Transform Transform => _transform ??= new Transform(ID);

        public bool HasRigidBody => InternalCalls.EntityHasRigidBody(ID);

        private RigidBody _rigidBody;
        public RigidBody RigidBody
        {
            get
            {
                if (_rigidBody == null)
                    _rigidBody = new RigidBody { EntityID = ID };  // assign same ID
                return _rigidBody;
            }
        }
        public bool HasCollider => InternalCalls.EntityHasCollider(ID);

        private Collider _collider;
        public Collider Collider
        {
            get
            {
                if (!HasCollider) return null;
                return _collider ??= new Collider { EntityID = ID };
            }
        }
        //some of these are placeholders/temp func
        // add component
        public void Add<T>() where T : class
        {
            if (typeof(T) == typeof(Transform))
                InternalCalls.AddTransform(ID);
            else if (typeof(T) == typeof(RigidBody))
                InternalCalls.AddRigidBody(ID);
            else if (typeof(T) == typeof(Collider))
                InternalCalls.AddCollider(ID);
            else if (typeof(T) == typeof(NameComponent))
                InternalCalls.AddName(ID);
            else
                //gotta keep make sure yk
                throw new Exception($"Add<{typeof(T).Name}> not supported!");
        }
        //remove component
        public void Remove<T>() where T : class
        {
            if (typeof(T) == typeof(Transform))
                InternalCalls.RemoveTransform(ID);
            else if (typeof(T) == typeof(RigidBody))
                InternalCalls.RemoveRigidBody(ID);
            else if (typeof(T) == typeof(Collider))
                InternalCalls.RemoveCollider(ID);
            else if (typeof(T) == typeof(NameComponent))
                InternalCalls.RemoveName(ID);
            else
                throw new Exception($"Remove<{typeof(T).Name}> not supported!");
        }

        //name component
        public bool HasName => InternalCalls.EntityHasName(ID)

        private NameComponent _nameComponent;
        public NameComponent NameComponent => _nameComponent ??= new NameComponent { EntityID = ID };

    }
}
