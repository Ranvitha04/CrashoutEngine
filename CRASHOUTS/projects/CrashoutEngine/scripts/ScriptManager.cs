/*!
@file       ScriptManager.cs
@author     Renfred Lee
@author     Shahrin Idzuan
@date       13/03/2026
@brief      Managed script registration and update dispatch.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Collections.Generic;

namespace CrashoutsEngine
{
    /*!
    @brief Tracks ScriptBase instances and dispatches their lifecycle callbacks.
    */
    public static class ScriptManager
    {
        private static List<ScriptBase> _scripts = new List<ScriptBase>();
        private static List<ScriptBase> _scriptsToRemove = new List<ScriptBase>();
        private static List<ScriptBase> _scriptsToAdd = new List<ScriptBase>();

        /*!
        @brief Register a script to be updated by the manager.
        @param script Script instance to register.
        */
        public static void RegisterScript(ScriptBase script)
        {
            _scriptsToAdd.Add(script);
        }

        /*!
        @brief Unregister a script so it will no longer be updated by the manager.
        @param script Script instance to unregister.
        */
        public static void UnregisterScript(ScriptBase script)
        {
            _scriptsToRemove.Add(script);
        }

        /*!
        @brief Apply pending register/unregister requests to the active list.
        */
        private static void SyncLists()
        {
            if (_scriptsToAdd.Count > 0)
            {
                foreach (var script in _scriptsToAdd)
                {
                    _scripts.Add(script);
                }
                _scriptsToAdd.Clear();
            }

            if (_scriptsToRemove.Count > 0)
            {
                foreach (var script in _scriptsToRemove)
                {
                    _scripts.Remove(script);
                }
                _scriptsToRemove.Clear();
            }
        }

        /*!
        @brief Call Update on all active scripts.
        */
        public static void UpdateAll()
        {
            SyncLists();
            foreach (var script in _scripts)
            {
                if (script?.Entity == null || !script.Entity.IsActive())
                    continue;

                // Note: We might need a way to check if the entity is still active
                // If the engine handles active state by not calling ScriptManager.UpdateAll, 
                // that's one thing, but here we are in C# loop.
                try {
                    script.Update();
                } catch (Exception e) {
                    Console.WriteLine($"[C# Error] Exception in {script.GetType().Name}.Update: {e.Message}");
                }
            }
        }

        /*!
        @brief Call LateUpdate on all active scripts.
        */
        public static void LateUpdateAll()
        {
            SyncLists();
            foreach (var script in _scripts)
            {
                if (script?.Entity == null || !script.Entity.IsActive())
                    continue;

                // Note: We might need a way to check if the entity is still active
                // If the engine handles active state by not calling ScriptManager.UpdateAll, 
                // that's one thing, but here we are in C# loop.
                try {
                    script.LateUpdate();
                } catch (Exception e) {
                    Console.WriteLine($"[C# Error] Exception in {script.GetType().Name}.LateUpdate: {e.Message}");
                }
            }
        }

        /*!
        @brief Call FixedUpdate on all active scripts.
        */
        public static void FixedUpdateAll()
        {
            SyncLists();
            foreach (var script in _scripts)
            {
                if (script?.Entity == null || !script.Entity.IsActive())
                    continue;

                try {
                    script.FixedUpdate();
                } catch (Exception e) {
                    Console.WriteLine($"[C# Error] Exception in {script.GetType().Name}.FixedUpdate: {e.Message}");
                }
            }
        }

        /*!
        @brief Call OnDrawGizmos on all registered scripts.
        */
        public static void OnDrawGizmosAll()
        {
            SyncLists();
            foreach (var script in _scripts)
            {
                try {
                    script.OnDrawGizmos();
                } catch (Exception e) {
                    Console.WriteLine($"[C# Error] Exception in {script.GetType().Name}.OnDrawGizmos: {e.Message}");
                }
            }
        }
        
        /*!
        @brief Clear all registered scripts and pending operations.
        */
        public static void ClearAll()
        {
            _scripts.Clear();
            _scriptsToAdd.Clear();
            _scriptsToRemove.Clear();
        }
    }
}
