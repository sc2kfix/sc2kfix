// sc2kfix include/setting_schema.h: setting defaults and schematic defines.
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#define CONF_GAME_SPEED_SETTING(x) (x - 1)

// Defaults

// SimCity2000

#define DEF_SIM_LOC_LANGUAGE      "USA"

#define DEF_SIM_OPT_AUTOBUDGET    0
#define DEF_SIM_OPT_AUTOGOTO      1
#define DEF_SIM_OPT_AUTOSAVE      0
#define DEF_SIM_OPT_DISASTERS     1
#define DEF_SIM_OPT_MUSIC         1
#define DEF_SIM_OPT_SOUND         1
#define DEF_SIM_OPT_SPEED         CONF_GAME_SPEED_SETTING(GAME_SPEED_LLAMA)

#define DEF_SIM_PATHS_CITIES      "Cities"
#define DEF_SIM_PATHS_DATA        "Data"
#define DEF_SIM_PATHS_GOODIES     "Goodies"
#define DEF_SIM_PATHS_GRAPHICS    "Bitmaps"
#define DEF_SIM_PATHS_MOVIES      "Movies"
#define DEF_SIM_PATHS_MUSIC       "Sounds"
#define DEF_SIM_PATHS_SCENARIOS   "Scenario"
#define DEF_SIM_PATHS_TILESETS    "ScurkArt"

#define DEF_SIM_REG_MAYOR_NAME    "Marvin Maxis"
#define DEF_SIM_REG_COMPANY_NAME  "Q37 Space Modulator Mfg."

#define DEF_SIM_SCRK_CYCLECOLORS  1
#define DEF_SIM_SCRK_GRIDHEIGHT   2
#define DEF_SIM_SCRK_GRIDWIDTH    2
#define DEF_SIM_SCRK_SHOWCLIPREG  0
#define DEF_SIM_SCRK_SHOWDRAWGRID 0
#define DEF_SIM_SCRK_SNAPTOGRID   0
#define DEF_SIM_SCRK_SOUND        1

#define DEF_SIM_VER_PROGS         256

#define DEF_SIM_WIN_DISPLAY       "8 1"
#define DEF_SIM_WIN_COLCHECK      0
#define DEF_SIM_WIN_LASTCOLDEPTH  32

// sc2kfix

#define DEF_FIX_AUD_ALWAYSPLAYMUSIC false
#define DEF_FIX_AUD_MASTERVOLUME    1.0
#define DEF_FIX_AUD_MUSICDRIVER     "fluidsynth"
#define DEF_FIX_AUD_MUSICINBKGRND   true
#define DEF_FIX_AUD_MUSICVOLUME     1.0
#define DEF_FIX_AUD_SHUFFLEMUSIC    false
#define DEF_FIX_AUD_SOUNDVOLUME     1.0
#define DEF_FIX_AUD_SOUNDFONT       "C:\\Windows\\System32\\drivers\\gm.dls"
#define DEF_FIX_AUD_USESNDREPLACE   true

#define DEF_FIX_CORE_CHECKFORUPD    true
#define DEF_FIX_CORE_FORCECON       false
#define DEF_FIX_CORE_INSTALLED      false
#define DEF_FIX_CORE_SKIPMODS       false

#define DEF_FIX_QOL_DARKUNDGRND     false
#define DEF_FIX_QOL_FREQUPDATES     true
#define DEF_FIX_QOL_SKIPINTRO       false
#define DEF_FIX_QOL_TITLECALEND     true
#define DEF_FIX_QOL_USEFLTSTATUS    false
#define DEF_FIX_QOL_USENEWSTRINGS   true

// Setting defines

// C_ - Category
// S_ - Section
// I_ - Item

// SimCity2000 categorised sections and items.

#define C_SIMCITY2000           "SimCity2000"

#define S_SIM_LOCALIZE          "Localize"
#define S_SIM_OPTIONS           "Options"
#define S_SIM_PATHS             "Paths"
#define S_SIM_REG               "Registration"
#define S_SIM_SCURK             "SCURK"
#define S_SIM_VER               "Version"
#define S_SIM_WIN               "Windows"

#define I_SIM_LOC_LANG          "Language"

#define I_SIM_OPT_AUTOBUDGET    "AutoBudget"
#define I_SIM_OPT_AUTOGOTO      "AutoGoto"
#define I_SIM_OPT_AUTOSAVE      "AutoSave"
#define I_SIM_OPT_DISASTERS     "Disasters"
#define I_SIM_OPT_MUSIC         "Music"
#define I_SIM_OPT_SOUND         "Sound"
#define I_SIM_OPT_SPEED         "Speed"

#define I_SIM_PATHS_CITIES      "Cities"
#define I_SIM_PATHS_DATA        "Data"
#define I_SIM_PATHS_GOODIES     "Goodies"
#define I_SIM_PATHS_GRAPHICS    "Graphics"
#define I_SIM_PATHS_HOME        "Home"
#define I_SIM_PATHS_MUSIC       "Music"
#define I_SIM_PATHS_SAVEGAME    "SaveGame"
#define I_SIM_PATHS_SCENARIOS   "Scenarios"
#define I_SIM_PATHS_TILESETS    "TileSets"

#define I_SIM_REG_MAYORNAME     "Mayor Name"
#define I_SIM_REG_COMPANYNAME   "Company Name"

#define I_SIM_SCRK_CYCLECOLORS  "CycleColors"
#define I_SIM_SCRK_GRIDHEIGHT   "GridHeight"
#define I_SIM_SCRK_GRIDWIDTH    "GridWidth"
#define I_SIM_SCRK_SHOWCLIPREG  "ShowClipRegion"
#define I_SIM_SCRK_SHOWDRAWGRID "ShowDrawGrid"
#define I_SIM_SCRK_SNAPTOGRID   "SnapToGrid"
#define I_SIM_SCRK_SOUND        "Sound"

#define I_SIM_VER_SC2K          "SimCity 2000"
#define I_SIM_VER_SCURK         "SCURK"

#define I_SIM_WIN_DISPLAY       "Display"
#define I_SIM_WIN_COLCHECK      "Color Check"
#define I_SIM_WIN_LASTCOLDEPTH  "Last Color Depth"

// sc2kfix categorised sections and items.

#define C_SC2KFIX                 "sc2kfix"

#define S_FIX_AUDIO               "audio"
#define S_FIX_CORE                "core"
#define S_FIX_KEYBINDS            "keybinds"
#define S_FIX_MUSMID              "music_midi"
#define S_FIX_MUSMP3              "music_mp3"
#define S_FIX_PATHS               "paths"
#define S_FIX_QOL                 "qol"

#define I_FIX_AUD_ALWAYSPLAYMUSIC "always_play_music"
#define I_FIX_AUD_MASTERVOLUME    "master_volume"
#define I_FIX_AUD_MUSICDRIVER     "music_driver"
#define I_FIX_AUD_MUSICINBKGRND   "music_in_background"
#define I_FIX_AUD_MUSICVOLUME     "music_volume"
#define I_FIX_AUD_SHUFFLEMUSIC    "shuffle_music"
#define I_FIX_AUD_SOUNDVOLUME     "sound_volume"
#define I_FIX_AUD_SOUNDFONT       "soundfont"
#define I_FIX_AUD_USESNDREPLACE   "use_sound_replacements"

#define I_FIX_CORE_CHECKFORUPD    "check_for_updates"
#define I_FIX_CORE_FORCECON       "force_console"
#define I_FIX_CORE_INSTALLED      "installed"
#define I_FIX_CORE_SETSAVETIME    "settings_save_time"
#define I_FIX_CORE_SKIPMODS       "skip_mods"

#define I_FIX_PATHS_CITIES        "cities"
#define I_FIX_PATHS_TILESETS      "tilesets"

#define I_FIX_QOL_DARKUNDGRND     "dark_underground"
#define I_FIX_QOL_FREQUPDATES     "frequent_updates"
#define I_FIX_QOL_SKIPINTRO       "skip_intro"
#define I_FIX_QOL_TITLECALEND     "title_calendar"
#define I_FIX_QOL_USEFLTSTATUS    "use_floating_status"
#define I_FIX_QOL_USENEWSTRINGS   "use_new_strings"
