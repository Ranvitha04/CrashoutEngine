/*!
@file   entity.cs
@author Renfred Lee (69%), Renfred (RTIS) (31%)
@date   29/11/2025

@brief  class for entity in Crashouts Engine

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

using System;
using System.Numerics;
using System.Reflection;
using System.Collections.Generic;


namespace CrashoutsEngine
{
    public class Entity
    {
        public uint ID { get; private set; }
        private readonly Dictionary<Type, object> _componentCache = new();

        public Entity(uint id)
        {
            ID = id;
        }
        public RigidBodyComponent RigidBodyComponent => GetComponent<RigidBodyComponent>();
        public ColliderComponent ColliderComponent => GetComponent<ColliderComponent>();
        public ShapeComponent ShapeComponent => GetComponent<ShapeComponent>();
        public bool HasCollider => Has<ColliderComponent>();

        //some of these are placeholders/temp func
        // add component
        public void Add<T>() where T : class
        {
            var type = typeof(T);
            var attr = type.GetCustomAttribute<NativeComponentAttribute>();

            if (attr != null)
            {
                // Use reflection to call InternalCalls.<AddCall>(ID)
                var method = typeof(InternalCalls).GetMethod(
                    attr.AddCall,
                    BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic
                );

                if (method == null)
                    throw new Exception($"InternalCalls method '{attr.AddCall}' not found for {type.Name}.");

                method.Invoke(null, new object[] { ID });
                return;
            }
        }

        //remove component
        public void Remove<T>() where T : class
        {
            var type = typeof(T);
            var attr = type.GetCustomAttribute<NativeComponentAttribute>();

            if (attr != null)
            {
                // Use reflection to call InternalCalls.<RemoveCall>(ID)
                var method = typeof(InternalCalls).GetMethod(
                    attr.RemoveCall,
                    BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic
                );

                if (method == null)
                    throw new Exception($"InternalCalls method '{attr.RemoveCall}' not found for {type.Name}.");

                method.Invoke(null, new object[] { ID });
                return;
            }
        }

        public bool Has<T>() where T : class
        {
            var type = typeof(T);
            var attr = type.GetCustomAttribute<NativeComponentAttribute>();

            if (attr == null)
                throw new Exception($"Component {type.Name} missing NativeComponent attribute!");

            var method = typeof(InternalCalls).GetMethod(
                attr.HasCall,
                BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic
            );

            if (method == null)
                throw new Exception($"InternalCalls method '{attr.HasCall}' not found.");

            return (bool)method.Invoke(null, new object[] { ID });
        }

        public T GetComponent<T>() where T : class
        {
            // does this entity have T?
            if (!Has<T>())
                return null;

            // if cached return it
            if (_componentCache.TryGetValue(typeof(T), out var cached))
                return cached as T;

            // create wrapper instance
            var instance = Activator.CreateInstance<T>();

            // Try to set EntityID
            var prop = typeof(T).GetProperty("EntityID");
            var field = typeof(T).GetField("EntityID");

            if (prop == null && field == null)
                throw new Exception($"Component {typeof(T).Name} has no EntityID field or property!");

            // Set Property or Field
            if (prop != null)
                prop.SetValue(instance, ID);
            else
                field.SetValue(instance, ID);


            // Cache and return
            _componentCache[typeof(T)] = instance;
            return instance;
        }

        // Static helper to find an entity globally
        public static Entity Find(string name)
        {
            uint id = InternalCalls.FindEntityByName(name);
            if (id == uint.MaxValue)
            {
                Debug.Log("Null");
                return null; // Check against MaxValue
            }
            //Debug.Log("Weird" + name);

            return new Entity(id);
        }

        public T GetScript<T>() where T : ScriptBase
        {
            // 1. Get the class name (e.g., "GoalScript")
            string name = typeof(T).Name;

            // 2. Ask C++ for the instance
            object scriptObj = InternalCalls.GetScript(ID, name);

            // 3. Cast and return
            return scriptObj as T;
        }
        public void SetActive(bool active)
        {
            InternalCalls.SetActive(ID, active);
        }

        public bool IsActive()
        {
            return InternalCalls.GetActive(ID);
        }

        // Helper method for easy checking
        public bool IsValid()
        {
            return ID != 0;
        }

        ////name component
        //public bool HasName => InternalCalls.EntityHasName(ID);

        //private NameComponent _nameComponent;
        //public NameComponent NameComponent => _nameComponent ??= new NameComponent { EntityID = ID };

    }
}
