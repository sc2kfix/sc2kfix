# sc2kfix Scripting Language (SX2)
**DISCLAIMER:** This is a work in progress and will change a lot along with the structure of sc2kfix.

## What is SX2?
The sc2kfix Scripting Language is an imperative scripting language implemented by sc2kfix. Its syntax is a somewhat assembly-like extension of the sc2kfix console command syntax. SX2 scripts are loaded into memory when they are called, with label and variable assignment and calculation following before execution begins. Parsing and execution of SX2 scripts is done entirely within the sc2kfix console infrastructure; in fact, most SX2 commands can be executed via the sc2kfix console interface directly not entirely unlike the manner of a REPL.

The sc2kfix console can execute SX2 scripts on demand with an arbitrary number of nested script calls.