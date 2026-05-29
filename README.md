# Valve Workbench

A visual tool for PC, created with QT, for measuring thermionic valves using the Valve Wizard's Valve Analyser

The Valve Wizard's Valve Analyser is an Arduino-based valve tester which he explains here: https://valvewizard.co.uk/analyser_mk2.html

This application is a complement to the Firmware project (https://github.com/olivergardiner/ValveTesterFirmware) and enables the PC to control the Valve Analyser such that it can run tests and display the results graphically.

## Build Instructions

### Environment

The project uses QT6 and CMake, and can easily be built with QT Creator or VS Code. You will need to install QT with a QT6 Kit, and if you want to use the MSVC compilers then you will need to install Visual Studio and the Windows SDK (for the debugger).
