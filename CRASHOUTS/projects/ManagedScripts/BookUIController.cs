/*!
@file       BookUIController.cs
@author     b.mohamadafiq@digipen.edu
@date       04/02/2026
@brief      Handles the book UI interactions in the game.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class BookUIController : ScriptBase
    {
        public static BookUIController Instance;

       
        public string[] closedBookPieces =
        {
            "ClosedBook",
            "ResumeBtnGame",
            "SettingBtnGame",
            "ExitBtnGame"
        };

       
        public string[] openBookPieces =
        {
            "OpenBook",
            "ClosedOpenBookBtn",
            "Instruction",
             "Music",
             "SFX",
             "SFX_Clone1986",
            "Instruction1",
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

           
            HideAll();
        }

        public override void Update()
        {
            if (Input.IsKeyTriggered(Key.P))
            {
                
                if (openShown)
                {
                    ShowClosedBook();
                    return;
                }

               
                closedShown = !closedShown;
                openShown = false;

                SetActiveList(closedIDs, closedShown);
                SetActiveList(openIDs, false);
            }
        }

     
        public void ShowOpenBook()
        {
            closedShown = false;
            openShown = true;

            SetActiveList(closedIDs, false);
            SetActiveList(openIDs, true);
        }

        
        public void ShowClosedBook()
        {
            closedShown = true;
            openShown = false;

            SetActiveList(openIDs, false);
            SetActiveList(closedIDs, true);
        }

        
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

