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

## Installation
Create a subfolder named WagicSyntaxPlugin inside your NotePad++ plugin folder, copy inside it the file WagicSyntaxPlugin.dll and restart the Editor.

## Development
The code is developed using Visual Studio 2017. Building the code will generate a DLL which can be used by Notepad++. For convenience, Visual Studio copies the DLL into the Notepad++ plugin directory.

## License
This code is released under the [GNU General Public License version 2](http://www.gnu.org/licenses/gpl-2.0.txt).
