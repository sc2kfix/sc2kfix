# sc2kfix
A bugfix DLL for SimCity 2000 Special Edition

## What does this do?
This is a DLL that patches the Special Edition (and 1995 CD Collection) release of SimCity 2000 for Windows 95 to work properly on modern Windows systems (Windows 7-11 and Wine/Proton). While the game itself is capable of running at high resolutions, oversights in the game's programming and techniques designed for use with 256-colour SVGA cards common in the mid 1990s cause problems with animations on truecolour displays. It also resolves the crashes that can occasionally happen with the load/save dialog boxes using a similar technique to Aleksander Krimsky's SC2000X patcher, but sc2kfix does so on in memory instead of creating a patched copy of your original EXE files.

The following issues are patched by this DLL:
* Replaces non-functional 16-bit installer by updating registry on first run
* Load/save dialog crash bug
* Frozen palette cycling animations

## How do I use it?
1. If you are installing the game from scratch, copy the SC2K folder from your CD (under the WIN95 folder in the Special Edition CD) to your hard drive.
   * Make sure to copy the SC2K folder somewhere writable, as the game stores its saves in a subfolder of that directory.
2. [Download the latest release](https://github.com/araxestroy/sc2kfix/releases) or build it from source. The whole plugin is a single file called `winmm.dll`.
3. Copy the `winmm.dll` to your SC2K folder. It should end up in the same folder as your `simcity.exe` executable.
   * If you're running the game on Wine/Proton, you'll need to set a DLL override for `simcity.exe` to use "native, then builtin" load order for the `winmm` library, otherwise Wine/Proton will not load the local `winmm.dll`.
4. Play the game! On the first run of a new install, the game will prompt you for a mayor and organization name. This simulates the original SimCity 2000 installer.


## To-do/known issues
* 1995 BUG: Clicking the "Load Saved City" option and then canceling the dialog box crashes the game
* TODO: Fix animations in SCURK as well
