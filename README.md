# sc2kfix
![A GIF of SimCity 2000's palette animations in action, with the sc2kfix logo overlaid on top of it.](https://i.imgur.com/lWEMFiT.gif)

## What does this do?
sc2kfix is a plugin that patches the Special Edition release of SimCity 2000 for Windows 95 to work properly on modern Windows systems (Windows 7-11 and Wine/Proton) as well as fixing several original game bugs, adding quality of life features, restoring missing game content, and optionally enabling new gameplay features. While the game itself was always capable of running at high resolutions, oversights in the game's programming and techniques designed for use with 256-colour SVGA cards common in the mid 1990s cause problems with animations on truecolour displays. There are also a number of other bugs in the game ranging from minor rendering issues to major gameplay-damaging bugs that sc2kfix fixes.

**For the most up-to-date information on how to install and configure sc2kfix, as well as a full list of features, check out the plugin user guide on the sc2kfix website: https://sc2kfix.net**

### Bugs fixed
The following game bugs are patched by this DLL:
* The old, non-functional installer is no longer needed as sc2kfix will prompt for a default mayor and company name on first launch.
* Multiple crash bugs related to the load and save dialogs have been fixed.
* Palette cycling based animations have been fixed to work on truecolour displays and are no longer frozen.
* SimCity 2000 now properly launches when started from Steam or other game launchers.
* Music that was included in the game but was never added to the random playlist has been restored.
* Military bases now grow properly instead of staying as empty, unusable military zones, and the intended growth of army and naval bases has been restored.
* The military will make multiple attempts at finding a location for a base instead of permanently giving up after the first try.
* Rail and highway neighbor connections now work after a saved city is loaded.
* Sign rendering has been fixed to use the originally intended font.
* City and mayor names are now properly preserved when saving and loading cities.
* The "Add All Inventions" option in the game's hidden debug menu has been fixed.

### New features
sc2kfix adds the following quality of life and optional gameplay features to SimCity 2000 Special Edition:
* A settings dialog for configuring sc2kfix's optional features has been added to the main menu and the in-game Options menu.
* The New City dialog has been updated to allow you to specify a different mayor name when starting a city.
* Higher quality copies of the transit, pipe, and power construction sounds, as well as the Reticulating Splines soundbite have been ported from other versions of SimCity 2000.
* City growth is now displayed in real-time instead of in batches when certain tools are used.
* The full in-game date is now shown in the title bar.
* The floating status widget from the Macintosh and DOS versions of the game has been reimplemented and can be enabled in the settings dialog.
* The game's music engine has been rewritten with multithreading that can run in the background and can play MP3 versions of the soundtrack if present.
* The middle mouse button now acts as a shortcut for the centering tool, similar to how the right mouse button works in the DOS version of the game.
* A number of message box strings have had grammatical fixes and/or better wording added to them.
* An update checker has been added to gently inform the player on the game's main menu if a new version of sc2kfix has been released.
* A debugging console has been added for experiments and advanced troubleshooting, as well as an `sc2kfix.log` file for informational and error logging.
* Multiple cheat codes from other ports of the game have been restored.

### Additional enhancements
sc2kfix implements multiple hooks and a work-in-progress framework for detouring and injecting new code into the game. These are documented in the hooks/hooks.md file. These are currently being used to assist in reverse engineering various components of the game's code, but attaching them to eg. a scripting language of some sorts in the future is not entirely unlikely. A framework to enable loading gameplay mods written in C/C++ (or any language that can produce Win32 DLLs, though currently only C/C++ are supported) is being implemented in this development branch, with intent to support various levels of code hooking and gameplay updates for Release 10.

There is also a debugging console that can be enabled by passing "-console" to SimCity 2000 or by enabling the console manually in the in-game "sc2kfix Settings" dialog. While the debugging console generally tries to stop you from doing anything too dangerous, it will happily let you probe any valid memory in the game's address space, which could have adverse effect on any loaded cities. Please be careful when writing to arbitrary memory addresses.

### Third-party components
sc2kfix includes the Kuroko programming language (https://github.com/kuroko-lang/kuroko) for modding and scripting purposes, which is available as a REPL in the sc2kfix console. Kuroko is an embeddable, bytecode-compiled dialect of Python that is released under the terms of the MIT license (copyright (c) 2020-2024 K. Lange <klange@toaruos.org>). Documentation on the language including an online REPL with a tutorial can be found at https://kuroko-lang.github.io.

### Third-party licenses
sc2kfix includes API bindings for version 2.4.7 of the FluidSynth real-time software synthesizer library (https://www.fluidsynth.org).. FluidSynth is made available under the terms of version 2.1 of the GNU Lesser General Public License (LGPL 2.1); a copy of this license has been included in the sc2kfix project as `thirdparty/lgpl-2.1-fluidsynth.txt`. You can download both the bindings and the requisite compiled version of the FluidSynth library to use them at https://github.com/FluidSynth/fluidsynth/releases/tag/v2.4.7.

## How do I use it?
1. If you are installing the game from scratch, copy the SC2K folder from your CD (under the WIN95 folder in the Special Edition CD) to your hard drive.
   * Make sure to copy the SC2K folder somewhere writable, as the game stores its saves in a subfolder of that directory.
2. [Download the latest release](https://github.com/araxestroy/sc2kfix/releases) or build it from source. The whole plugin is a single file called `winmm.dll`.
3. Copy the `winmm.dll` to your SC2K folder. It should end up in the same folder as your `simcity.exe` executable.
   * If you're running the game on Wine/Proton, you'll need to set a DLL override for `simcity.exe` to use "native, then builtin" load order for the `winmm` library, otherwise Wine/Proton will not load the local `winmm.dll`.
   * If you have any mods to install, place them in a subdirectory named `mods` in the same older as `simcity.exe`.
4. Play the game! On the first run of a new install, the game will prompt you for a default mayor and organization name. This simulates the original SimCity 2000 installer.

You can configure different aspects of sc2kfix by clicking on the "sc2kfix Settings" button on the game's main menu, or while you're in a gameplay session by selecting the same option from the Options menu. Settings marked with an asterisk require restarting the game for them to take effect. sc2kfix defaults to providing a "vanilla plus" gameplay experience with bug fixes and minor quality of life improvements; gameplay-modifying features are opt-in through the settings dialog, and quality of life features can be opted out of the same way.

## Known issues
* NOTICE: The 1995 CD Collection version of the game is now deprecated. Please use the 1996 Special Edition version of the game going forward to receive the latest updates.
* BUG: Something may have changed in Windows 11 24H2 that breaks SCURK's pick-and-place tool; this is being investigated.
* DEBUG: Experimental support for the 1995 Interactive Demo of the game has been added. This is not intended to support the full feature set of sc2kfix, and is only meant to help with ascertaining differences between the demo and release versions of the game.
