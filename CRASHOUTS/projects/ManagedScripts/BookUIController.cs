using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class BookUIController : ScriptBase
    {
        public static BookUIController Instance;

        // Must match your hierarchy names EXACTLY
        public string[] closedBookPieces =
        {
            "ClosedBook",
            "ResumeBtnGame",
            "SettingBtnGame",
            "ExitBtnGame"
        };

        public string[] openBookPieces =
        {
            "OpenBook"
            // Add OpenBook children here if you have them:
            // "OpenBackBtn", "OpenSomething", ...
        };

        public float shownAlpha = 1.0f;
        public float hiddenAlpha = 0.0f; // 0.2f if you want dim instead of invisible

        private uint[] closedIDs;
        private uint[] openIDs;

        private bool closedShown = false;

        public override void Start()
        {
            Instance = this;

            closedIDs = CacheIDs(closedBookPieces);
            openIDs = CacheIDs(openBookPieces);

            // Hide everything at start
            SetAlphaList(closedIDs, hiddenAlpha);
            SetAlphaList(openIDs, hiddenAlpha);
        }

        public override void Update()
        {
            if (Input.IsKeyTriggered(Key.P))
            {
                closedShown = !closedShown;

                // Toggle ClosedBook
                SetAlphaList(closedIDs, closedShown ? shownAlpha : hiddenAlpha);

                // When showing ClosedBook, keep OpenBook hidden
                if (closedShown)
                    SetAlphaList(openIDs, hiddenAlpha);
            }
        }

        public void ShowOpenBook()
        {
            closedShown = false;
            SetAlphaList(closedIDs, hiddenAlpha);
            SetAlphaList(openIDs, shownAlpha);
        }

        public void ShowClosedBook()
        {
            closedShown = true;
            SetAlphaList(openIDs, hiddenAlpha);
            SetAlphaList(closedIDs, shownAlpha);
        }

        private uint[] CacheIDs(string[] names)
        {
            uint[] ids = new uint[names.Length];
            for (int i = 0; i < names.Length; i++)
            {
                ids[i] = InternalCalls.FindEntityByName(names[i]);
                Debug.Log(names[i] + " id = " + ids[i]);
            }
            return ids;
        }

        private void SetAlphaList(uint[] ids, float a)
        {
            for (int i = 0; i < ids.Length; i++)
            {
                if (ids[i] == uint.MaxValue) continue;

                Entity e = new Entity(ids[i]);
                ShapeComponent sc = e.GetComponent<ShapeComponent>();
                if (sc == null) continue;

                sc.Alpha = a;
            }
        }
    }
}
