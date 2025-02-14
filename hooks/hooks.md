# sc2kfix Hooking Infrastructure
**DISCLAIMER:** This is a work in progress and will change a lot along with the structure of sc2kfix.

## What kind of hooks do we use?
sc2kfix uses three different types of hooks to work its various magicks, which are described in this file.

### Type 1: WinMM import hooks
Since sc2kfix works by being locally loaded as winmm.dll before the system's own DLL of the same name, it can intercept any calls to the real winmm.dll instead of directly passing them through. This is incredibly lightweight as we already pretend to be the whole winmm.dll, so intercepting a call is as simple as copying the framework and moving the corresponding export ordinal to the `ALLEXPORTS_HOOKED()` macro.

Currently, sc2kfix intercepts the following winmm.dll calls and performs its own processing before passing data through to the system winmm.dll library:
- `mciSendCommandA()`
- `sndPlaySoundA()`
- `timeSetEvent()`

### Type 2: FARPROC call thunk hooks
SimCity 2000 for Windows was written using an early MFC library that uses a lot of thunks to man-in-the-middle relative offset calls with `jmp imm32` instructions. This is extremely useful for us as we can overwrite those `jmp imm32` instructions to point at our own code with the `NEWJMP()` macro, and after we're done we can just call back into the original SimCity 2000 code and return to the caller. Starting in 0.6-dev there are also macros for doing the same but with conditional branch instructions.

Currently, sc2kfix hooks the following call thunks this way:
- 0x401F9B (`jmp 0x480140`) -- intercepts loading WAV files into the SC2K internal sound buffers

### Type 3: FARPROC call hooks, direct style
We can also intercept calls directly with the `NEWCALL()` macro. It works similarly but by inserting a `call imm32` instruction (0xE8) instead of a `jmp imm32` instruction (0xE9).

We aren't currently using this anywhere.


## What do these hooks look like?

### Hook structure
Each formalized hook as injected in place of the original takes the same parameters as said original call. Two hook functions, one before and one after the original call, are called with return type `BOOL` and two additional leading parameters (a `void*` to the original call return address and a pointer to a working return value of the original call's return type). In between these, the original function is called. If the "before hook" returns `FALSE`, the original call is discarded, and the original SimCity 2000 caller is returned to as if the original function call had failed.

Example:
1. SimCity 2000 (the caller) calls `sndPlaySoundA(...)`.
2. sc2kfix's interception reaches `_sndPlaySoundA(...)` in `hook_sndPlaySound.cpp`.
3. `Hook_sndPlaySoundA(void* pReturnAddress, BOOL* retval, ...)` is called with `_ReturnAddress()` as the first added parameter and `&retval` as the second.
4. If `Hook_sndPlaySoundA()` returns `FALSE`, all following code is skipped and `_sndPlaySoundA()` returns to the caller.
5. Otherwise, the real `sndPlaySoundA()` is called with the original parameters, and its return value is placed in `retval`.
6. `HookAfter_sndPlaySoundA(void* pReturnAddress, BOOL* retval, ...)` is called with `_ReturnAddress()` as the first added parameter and `&retval` as the second.
7. The original caller is returned to with the contents of `retval` as the return code of `sndPlaySoundA()`.


## What other cool stuff that aren't hooks do we have?
The header file `include/sc2k_1996.h` is your friend. Inside there are enums, structs, and a whole bunch of reference pointer definitions that let the native code in sc2kfix manipulate game state as if it's operating in the game thread itself. This is a new feature as of 0.6-dev and is probably something you don't want to mess with unless you really know what you're doing.