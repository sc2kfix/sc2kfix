# sc2kfix
![A GIF of SimCity 2000's palette animations in action.](https://i.imgur.com/DYl508z.gif)

## What does this do?
This is a DLL that patches the Special Edition release of SimCity 2000 for Windows 95 to work properly on modern Windows systems (Windows 7-11 and Wine/Proton) as well as fixing bugs and optionally enabling new features. While the game itself is capable of running at high resolutions, oversights in the game's programming and techniques designed for use with 256-colour SVGA cards common in the mid 1990s cause problems with animations on truecolour displays. It also resolves the crashes that can occasionally happen with the load/save dialog boxes using a similar technique to Aleksander Krimsky's SC2000X patcher, but sc2kfix does so on in memory instead of creating a patched copy of your original EXE files.

The following issues are patched by this DLL:
* Replaces non-functional 16-bit installer by updating registry on first run
* Load/save dialog crash bug
* Frozen palette cycling animations
   * This this has been fixed in both the 1995 CD Collection and 1996 Special Edition of SimCity 2000, as well as the 1996 Special Edition version of SCURK.
* Military bases now grow properly instead of being permanently empty
* The military will make multiple attempts at finding a location for a base instead of giving up after the first try

For the 1996 Special Edition version of SimCity 2000, sc2kfix also implements multiple hooks and a work-in-progress framework for detouring and injecting new code into the game. These are documented in the hooks/hooks.md file. These are currently being used to assist in reverse engineering various components of the game's code, but attaching them to eg. a scripting language of some sorts in the future is not entirely unlikely. There is also a debugging console that can be enabled by passing "-console" to SimCity 2000. While the debugging console generally tries to stop you from doing anything too dangerous, it will happily let you probe any valid memory in the game's address space, which could have adverse effect on any loaded cities. Please be careful when writing to arbitrary memory addresses.
* Note: It is not likely that this will ever be fully functional on the 1995 CD Collection version of the game as that would require reverse engineering two different builds of the same game for minimal practical improvement.

## How do I use it?
1. If you are installing the game from scratch, copy the SC2K folder from your CD (under the WIN95 folder in the Special Edition CD) to your hard drive.
   * Make sure to copy the SC2K folder somewhere writable, as the game stores its saves in a subfolder of that directory.
2. [Download the latest release](https://github.com/araxestroy/sc2kfix/releases) or build it from source. The whole plugin is a single file called `winmm.dll`.
3. Copy the `winmm.dll` to your SC2K folder. It should end up in the same folder as your `simcity.exe` executable.
   * If you're running the game on Wine/Proton, you'll need to set a DLL override for `simcity.exe` to use "native, then builtin" load order for the `winmm` library, otherwise Wine/Proton will not load the local `winmm.dll`.
4. Play the game! On the first run of a new install, the game will prompt you for a mayor and organization name. This simulates the original SimCity 2000 installer.

You can configure different aspects of sc2kfix by clicking on the "sc2kfix Settings" button on the game's main menu, or while you're in a gameplay session by selecting the same option from the Options menu. Settings marked with an asterisk require restarting the game for them to take effect.

## To-do/known issues
* 1995 BUG: Clicking the "Load Saved City" option and then canceling the dialog box crashes the game
* 1995 TODO: Fix animations in SCURK 1995
* BUG: Something may have changed in Windows 11 24H2 that breaks SCURK's pick-and-place tool; this is being investigated.
