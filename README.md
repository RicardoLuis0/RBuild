# RBuild

Simple declarative buildsystem for C/C++, supports windows and linux platforms, should be trivial to port to other unix-like systems.

It is built to be as simple as viable, and as such it doesn't handle:
* Dependency discovery
* Pre/Post build steps
* etc

It does support:
* Specifying link order
* Specifying sources either per-file or per-folder ( as white/blacklist )
* Multiple targets (be them debug/release/etc)

The format for the json file is specified in [`info/json_layout.txt`](info/json_layout.txt), and the commandline arguments the program accepts are:
* `-file=[json file]` to specify which file it uses as the project file, if no file is specified it uses "[current folder name].json"
* `-rebuild` rebuild project from scratch, ignore already-built object files
* `-verbose` print whole command line of executed compilers/linkers

## Using

_**TODO**_

## Building

This project doesn't have any dependencies other than a recent-enough version of gcc (with C++20 support), and it can be built either with itself, or via a shell script.

### Linux
#### Option 1 (shell script)
* run `build.sh` to build it
* optinally run `install.sh` to add it to your path
#### Option 2 (codeblocks)
* open the `RBuild.cbp` project file in codeblocks, and compile it
* optionally manually add it to your path
#### Option 3 (itself)
* run `RBuild` on the project folder, if the project is in a folder named `RBuild`, or `RBuild -file=RBuild.json` if the project is in a different folder
* optinally run `install.sh` to add it to your path
### Windows
#### Option 1 (batch file)
* run `build.bat` to build it (requires MinGW's `g++` to be on your path)
* optionally manually add it to your path
#### Option 2 (codeblocks)
* open the `RBuild.cbp` project file in codeblocks, and compile it
* optionally manually add it to your path
#### Option 3 (itself)
* run `RBuild` on the project folder, if the project is in a folder named `RBuild`, or `RBuild -file=RBuild.json` if the project is in a different folder
* optionally manually add it to your path
