#pragma once

// The SC2K resource IDs are set here to make working with them slightly less of a headache.
// For the most part these are general across all Windows (95?) versions of the game.
// Any special cases will be noted down accordingly.

// This macro is used if you're accessing game menu positional indices
// via CMainFrame rather than CSimcityView.
#define SC2K_MENU_GAME_FROM_MAIN(x) (x + 1)

// Dialog, Item IDs
#define SC2K_DIALOG_NEWCITY       101
#define SC2K_DIALOG_BUDGET        102
#define SC2K_DIALOG_MAIN          103
#define SC2K_DIALOG_SELECTITEM    113
#define SC2K_DIALOG_QUERYGENERAL  142
#define SC2K_DIALOG_QUERYSPECIFIC 154

// Menu IDs
#define SC2K_MENU_MAIN            2
#define SC2K_MENU_GAME            3
#define SC2K_MENU_CITYTOOLBAR     136
#define SC2K_MENU_RCLKSHORTCUT    243

// Sub-menu IDs
// MainFrame
#define SC2K_MENU_MAIN_FILE       0
#define SC2K_MENU_MAIN_HELP       1
// SimcityView
#define SC2K_MENU_GAME_FILE       0
#define SC2K_MENU_GAME_SPEED      1
#define SC2K_MENU_GAME_OPTIONS    2
#define SC2K_MENU_GAME_DISASTERS  3
#define SC2K_MENU_GAME_WINDOWS    4
#define SC2K_MENU_GAME_PAPERS     5
#define SC2K_MENU_GAME_HELP_NODBG 6 // This is the position if the debug menu isn't attached and active.
#define SC2K_MENU_GAME_DEBUG      6
#define SC2K_MENU_GAME_HELP       7

// String IDs
