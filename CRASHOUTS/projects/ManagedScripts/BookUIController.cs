using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class BookUIController : ScriptBase
    {
        public static BookUIController Instance;

        // Must match your hierarchy names EXACTLY (UNIQUE NAMES ONLY)
        public string[] closedBookPieces =
        {
            "ClosedBook",
            "ResumeBtnGame",
            "SettingBtnGame",
            "ExitBtnGame"
        };

        // Put EVERY OpenBook UI entity name here (including clones)
        public string[] openBookPieces =
        {
            "OpenBook",
            "ClosedOpenBookBtn",
            "Instruction",
            "Instruction2",
            "Instruction3",
            "Instruction4",
            "Instruction5",
            "Instruction6",
            "Instruction7",
            "Instruction8",
            "Instruction9",
            "Instruction10",
            "Instruction11",
            "Music",
            "SFX",
            "Slider",
            "Slider_Clone2001",

   
        };

        private uint[] closedIDs;
        private uint[] openIDs;

        private bool closedShown = false;
        private bool openShown = false;

        public override void Start()
        {
            Instance = this;

            closedIDs = CacheIDs(closedBookPieces);
            openIDs = CacheIDs(openBookPieces);

            // Hide everything at start
            HideAll();
        }

        public override void Update()
        {
            if (Input.IsKeyTriggered(Key.P))
            {
                // OPTION B:
                // If OpenBook is open, P goes BACK to ClosedBook
                if (openShown)
                {
                    ShowClosedBook();
                    return;
                }

                // Otherwise P toggles ClosedBook
                closedShown = !closedShown;
                openShown = false;

                SetActiveList(closedIDs, closedShown);
                SetActiveList(openIDs, false);
            }
        }

        // ClosedBook -> OpenBook
        public void ShowOpenBook()
        {
            closedShown = false;
            openShown = true;

            SetActiveList(closedIDs, false);
            SetActiveList(openIDs, true);
        }

        // OpenBook -> ClosedBook
        public void ShowClosedBook()
        {
            closedShown = true;
            openShown = false;

            SetActiveList(openIDs, false);
            SetActiveList(closedIDs, true);
        }

        // Hide everything (Resume)
        public void HideAll()
        {
            closedShown = false;
            openShown = false;

            if (closedIDs != null) SetActiveList(closedIDs, false);
            if (openIDs != null) SetActiveList(openIDs, false);
        }

        private uint[] CacheIDs(string[] names)
        {
            uint[] ids = new uint[names.Length];
            for (int i = 0; i < names.Length; i++)
            {
                ids[i] = InternalCalls.FindEntityByName(names[i]);
                Debug.Log("[BookUI] " + names[i] + " id = " + ids[i]);
            }
            return ids;
        }

        private void SetActiveList(uint[] ids, bool active)
        {
            for (int i = 0; i < ids.Length; i++)
            {
                if (ids[i] == uint.MaxValue) continue;
                InternalCalls.SetActive(ids[i], active);
            }
        }
    }
}
