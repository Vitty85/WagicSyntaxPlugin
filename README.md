# WagicSyntaxPlugin
The Wagic Primitives Syntax Plugin for Notepad++

## Usage
The Plugin starts enabled, but you can disable it by using the plugin menu choice "Disable Online Syntax Check"

If the Plugin is disabled, you can activate it by using the plugin menu choice "Enable and Perform Current Line Syntax Check"

The Plugin will automatically detects keywords, triggers, zones, macros and constants and it will highlights them using different colors:
- Blue style for keywords
- Green style for comments
- Bordeaux style for triggers
- Purple style for zones
- Orange style for macros
- Gray style for macros

The Plugin will automatially suggests Wagic keywords, triggers, zones, macros and constants by the popup menu (you can open it by pressing CTRL+SPACE).

The Plugin, by default, will performs inline syntax check on the current row, to activate the full file check you can use the choice "Enable and Perform All Lines Syntax Check", but this option can heavily slow down the editor if the file is very big.

The Plugin will also automatially detect unbalanced parenthesis in the current row (or file) and it will highlight them with red color, and this is true for '()', '[]', '{}', '$$' and '!!' couples.

## Installation
Donwload the relased zip file containing the WagicSyntaxPlugin.dll and unzip it inside your NotePad++ plugin folder, then restart the editor.

BE AWARE: Currently the release is compiled for 64bit version, to use it on 32bit version you should recoimpile the DLL on the target architecture.

## Development
The code is developed using Visual Studio 2019. Building the code will generate a DLL which can be used by Notepad++. For convenience, Visual Studio copies the DLL into the Notepad++ plugin directory.

## License
This code is released under the [GNU General Public License version 2](http://www.gnu.org/licenses/gpl-2.0.txt).
