#include "gui.h"

#include <algorithm>

#include "../interfaces.h"

// Javascript to force root panel to have our child and raise it.
const char *cuckProtocol =
                "var parentPanel = $.GetContextPanel();\n"
                "var mcDota = parentPanel.FindChild(\"McDotaMain\");\n"
                "$.Msg(\"Loading Panel: \" + mcDota.id);"
                "mcDota.BLoadLayoutFromString( \"%s\", false, false);\n";

static constexpr unsigned int JS_MAX = 65535;
char jsCode[JS_MAX];
std::string mainXML =
#include "main.xml"
;

static panorama::IUIPanel* GetHudRoot( ){
    panorama::IUIPanel *panel = panoramaEngine->AccessUIEngine()->GetLastDispatchedEventTargetPanel();

    if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer(panel) ){
        cvar->ConsoleDPrintf("[GUI::GetHudRoot]Failed to grab Last Event Target Panel!\n");
        return NULL;
    }
    panorama::IUIPanel *itr = panel;
    panorama::IUIPanel *ret = nullptr;
    while( itr && panoramaEngine->AccessUIEngine()->IsValidPanelPointer(itr) ){
        if( !strcasecmp(itr->GetID(), "DOTAHud") ){
            ret = itr;
            break;
        }
        itr = itr->GetParent();
    }
    return ret;
}
static bool SetupAndCheckPanels()
{
    /* Grab needed root panel if we don't have it already */
    if( engine->IsInGame() ){
        panorama::IUIPanel* panel = GetHudRoot();
        if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer( panel ) ){
            cvar->ConsoleDPrintf( "Could not Get HUD Root Panel! Invalid! (%p)\n", (void*)UI::hudRoot );
            return false;
        }
        UI::hudRoot = panel;
    } else if( !UI::dashRoot ){
        panorama::IUIPanel *panel = panoramaEngine->AccessUIEngine()->GetPanelArray()->slots[0].panel; // 0 = DotaDashboard
        if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer( panel ) ){
            cvar->ConsoleDPrintf( "Could not Get Dashboard Root Panel! Invalid! (%p)\n", (void*)UI::dashRoot );
            return false;
        }
        UI::dashRoot = panel;
    }
    /* Are we in-game? The Root panel is different */
    panorama::IUIPanel *root = ( engine->IsInGame() ? UI::hudRoot : UI::dashRoot );

    if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer(root) ){
        cvar->ConsoleDPrintf("[UI::SetupAndCheckPanels] - Root panel pointer Invalid(%p)\n", (void*)root);
        return false;
    }
    if( !root->HasBeenLayedOut() ){
        cvar->ConsoleDPrintf("[UI::SetupAndCheckPanels] - Root panel has not been layed out yet!\n");
        return false;
    }
    /* Going from menu->In-game OR Vice versa, set the pointer to NULL so we re-grab it below */
    if( panoramaEngine->AccessUIEngine()->IsValidPanelPointer( UI::mcDota ) ){
        if( (engine->IsInGame() && UI::mcDota->GetParent() != UI::hudRoot) ||
            (!engine->IsInGame() && UI::mcDota->GetParent() != UI::dashRoot) ){
            UI::mcDota = NULL;
        }
    }
    /* Going from in-game back to menu, change the pointer to our existing panel if it is there. */
    if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer( UI::mcDota ) && !engine->IsInGame() ){
        panorama::IUIPanel* child = UI::dashRoot->GetLastChild();
        if( panoramaEngine->AccessUIEngine()->IsValidPanelPointer( child ) && strcmp(child->GetID(), "McDotaMain") == 0 ){
            cvar->ConsoleDPrintf("Grabbing existing child McDotaMain\n");
            UI::mcDota = child;
        }
    }
    /* Create our custom panel */
    if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer( UI::mcDota ) ){
        cvar->ConsoleDPrintf("Creating McDota Panel...\n");
        // Get rid of newlines, they mess up the javascript syntax
        std::replace(mainXML.begin(), mainXML.end(), '\n', ' ');
        snprintf(jsCode, JS_MAX, cuckProtocol, mainXML.c_str());

        panorama::CPanoramaSymbol type = panoramaEngine->AccessUIEngine()->MakeSymbol("Panel");
        UI::mcDota = panoramaEngine->AccessUIEngine()->CreatePanel(&type, "McDotaMain", root)->panel;
        cvar->ConsoleDPrintf("Root ID: %s\n", root->GetID());
        UI::mcDota->SetParent( root );
    }
    if( !UI::mcDota->HasBeenLayedOut() )
        panoramaEngine->AccessUIEngine()->RunScript(root, jsCode, engine->IsInGame() ? "panorama/layout/base_hud.xml" : "panorama/layout/base.xml", 8, 10, false);

    return true;
}

void UI::ToggleUI()
{
    SetupAndCheckPanels();

    if( !panoramaEngine->AccessUIEngine()->IsValidPanelPointer(UI::mcDota) ){
        cvar->ConsoleDPrintf("[UI::ToggleUI] - Something is wrong with our mcDota Panel Pointer(%p)\n", (void*)UI::mcDota);
        return;
    }
    if( !UI::mcDota->HasBeenLayedOut() ){
        cvar->ConsoleDPrintf("[UI::ToggleUI] - mcDota Panel not layed out yet. Try again.\n");
        return;
    }

    UI::mcDota->SetVisible(!UI::mcDota->IsVisible());

    /* Play Menu Open/Close sounds */
    if( UI::mcDota->IsVisible() )
        panoramaEngine->AccessUIEngine()->RunScript(UI::mcDota, "$.DispatchEvent( 'PlaySoundEffect', 'ui_menu_activate_open' );", engine->IsInGame() ? "panorama/layout/base_hud.xml" : "panorama/layout/base.xml", 8, 10, false );
    else
        panoramaEngine->AccessUIEngine()->RunScript(UI::mcDota, "$.DispatchEvent( 'PlaySoundEffect', 'ui_menu_activate_close' );", engine->IsInGame() ? "panorama/layout/base_hud.xml" : "panorama/layout/base.xml", 8, 10, false );

}