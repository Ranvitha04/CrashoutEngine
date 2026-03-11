using System;
using System.Collections.Generic;

namespace CrashoutsEngine
{
    public static class ScriptManager
    {
        private static List<ScriptBase> _scripts = new List<ScriptBase>();
        private static List<ScriptBase> _scriptsToRemove = new List<ScriptBase>();
        private static List<ScriptBase> _scriptsToAdd = new List<ScriptBase>();

        public static void RegisterScript(ScriptBase script)
        {
            _scriptsToAdd.Add(script);
        }

        public static void UnregisterScript(ScriptBase script)
        {
            _scriptsToRemove.Add(script);
        }

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
        
        public static void ClearAll()
        {
            _scripts.Clear();
            _scriptsToAdd.Clear();
            _scriptsToRemove.Clear();
        }
    }
}
