# WagicSyntaxPlugin
The Wagic Primitives Syntax Plugin for Notepad++

## Usage
The Plugin starts enabled by default, but you can disable it by using the plugin menu choice "Disable Online Syntax Check (SHIFT+SPACEBAR)"

When the Plugin is disabled, you can activate it by using the plugin menu choice "Enable and Perform Visible Lines Syntax Check (ALT+SPACEBAR)"

The Plugin will automatically detect keywords, triggers, zones, macros and constants and it will highlight them using different colors:
- Blue style for keywords
- Green style for comments
- Bordeaux style for triggers
- Purple style for zones
- Orange style for macros
- Gray style for constants and basic abilities
- Red style for unknown or wrong words

The Plugin will automatically suggest Wagic keywords, triggers, zones, macros and constants by the Notepad++ suggestions popup menu (you can open it by pressing CTRL+SPACEBAR).

When enabled, the Plugin will perform by default an inline syntax check on the visible rows, to activate the full file check you can use the choice "Enable and Perform All Lines Syntax Check", but this option can heavily slow down the editor when the file is very big.

The Plugin will automatically detect unbalanced brackets in the current row (or file) and it will highlight them with red color, and this is true for '()', '[]', '{}', '$$' and '!!' couples.

## Installation
Download the released zip file containing the WagicSyntaxPlugin.dll in its own subfolder and unzip it inside your NotePad++ plugin folder, then restart the editor.

BE AWARE: Currently the release is compiled for 64bit version, to use it on 32bit version you should recompile the DLL on the target architecture.

## Development
The code is developed using Visual Studio 2019. Building the code will generate a DLL which can be used by Notepad++. For convenience, Visual Studio copies the DLL into the Notepad++ plugin directory.

## License
This code is released under the [GNU General Public License version 2](http://www.gnu.org/licenses/gpl-2.0.txt).
