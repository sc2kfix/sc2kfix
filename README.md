# sc2kfix
A bugfix DLL for SimCity 2000 Special Edition (1996)

## What does this do?
This is a DLL that patches the 1996 Special Edition release of SimCity 2000 for Windows 95 to work properly on modern Windows systems. While the game itself is capable of running at high resolutions, oversights in the game's programming and techniques designed for use with 256-colour SVGA cards common in the mid 1990s cause problems with animations on truecolour displays. It also resolves the crashes that can occasionally happen with the load/save dialog boxes using a similar technique to Aleksander Krimsky's SC2000X patcher, but sc2kfix does so on in memory instead of creating a patched copy of your original EXE files.

The following issues are patched by this DLL:
* Load/save dialog crash bug
* Frozen palette cycling animations

## How do I use it?
Simply copy the winmm.dll from the latest release to the same folder as simcity.exe, then start the game.
