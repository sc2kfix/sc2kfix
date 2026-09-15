#pragma once

// The SC2K resource IDs are set here to make working with them slightly less of a headache.
// For the most part these are general across all Windows (95?) versions of the game.
// Any special cases (non-native or differing) will be noted down accordingly.

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

#define SC2K_DIALOG_NEWCITY_BTN_REGENERATE    20   // Not native
#define SC2K_DIALOG_NEWCITY_EDIT_CITYNAME     101
#define SC2K_DIALOG_NEWCITY_RADIO_YEAR1900    104
#define SC2K_DIALOG_NEWCITY_RADIO_YEAR1950    105
#define SC2K_DIALOG_NEWCITY_RADIO_YEAR2000    106
#define SC2K_DIALOG_NEWCITY_RADIO_YEAR2050    107
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNCLASSIC 108 // Not native
#define SC2K_DIALOG_NEWCITY_RADIO_DIFFEASY    109
#define SC2K_DIALOG_NEWCITY_RADIO_DIFFMEDIUM  110
#define SC2K_DIALOG_NEWCITY_RADIO_DIFFHARD    111
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNGREY    112  // Not native
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNLUSH    113  // Not native
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNCOLD    114  // Not native
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNHOT     115  // Not native
#define SC2K_DIALOG_NEWCITY_RADIO_TRRNRANDOM  116  // Not native
#define SC2K_DIALOG_NEWCITY_LBL_TRRNWARN      117  // Not native
#define SC2K_DIALOG_NEWCITY_LBL_TIP           119  // Not native
#define SC2K_DIALOG_NEWCITY_EDIT_MAYORNAME    150  // Not native
#define SC2K_DIALOG_NEWCITY_LBL_DIFFEASY      1001 // Not native
#define SC2K_DIALOG_NEWCITY_LBL_DIFFMEDIUM    1002 // Not native
#define SC2K_DIALOG_NEWCITY_LBL_DIFFHARD      1003 // Not native
#define SC2K_DIALOG_NEWCITY_LBL_STARTYEAR     1010 // Not native
#define SC2K_DIALOG_NEWCITY_LBL_DIFFICULTY    1011 // Not native
#define SC2K_DIALOG_NEWCITY_LBL_TRRNTYPE      1012 // Not native

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
