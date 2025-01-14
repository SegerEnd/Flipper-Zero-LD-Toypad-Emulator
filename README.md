# Flipper Zero Lego Dimensions ToyPad Emulator
## Overview
This project aims to create a USB emulation app for the Flipper Zero to emulate the functionality of a Lego Dimensions ToyPad.

## Project Status
The project is currently in development. **Character placement is working** 🎉, but there are issues with vehicle emulation and the placement indexes for characters.

## Features
Placing characters on the ToyPad,
When the ToyPad is connected to the game, you can use the **arrow keys** to select a spot on the ToyPad

![Schermafbeelding 2025-01-14 163051](https://github.com/user-attachments/assets/e62fb2bd-8ee1-4b7e-9271-cc68068758d9)

And press **OK** to select a character from the list:

![Schermafbeelding 2025-01-14 163150](https://github.com/user-attachments/assets/9f47cb9d-1990-476e-adb0-3872d39496f8)

## Current Issues
I'm currently facing the following issues:

#### Vehicles:
Currenly vehicles are not working there is, I think a issue in the CMD_MODEL or CMD_READ or with the packet generation that is causing it not to work
#### Issue character removing
When removing a character from the ToyPad, the figure is removed, but the indexes are not reset/shifted correctly. Causing unexpected behavior afterwards.

## Code Reference
I have found a similar project that helped this project for insight and code snippets for the emulation. [Node LD](https://github.com/AlinaNova21/node-ld) & [LD-ToyPad-Emulator](https://github.com/Berny23/LD-ToyPad-Emulator) repository.

#
If you'd like to help complete this project, feel free to make a pull request or reach out to me!
