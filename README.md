# RBuild

Simple declarative buildsystem for C/C++, supports windows and linux out of the box, but should be trivial to port to other unix-like platforms.

It is built to be as simple as viable, and as such it **doesn't** handle:
* Dependency discovery
* Pre/Post build steps
* etc

It does support:
* Incremental Building
* Exact linking order, if necessary
* Specifying sources either per-file or per-folder ( as white/blacklist )
* Multiple targets ( be them debug/release/etc )

## Using

The command-line arguments the program accepts are:
* `-file=[json file]` to specify which file it uses as the project file, if no file is specified it uses "[current folder name].json"
* `-rebuild` rebuild project from scratch, ignore already-built object files
* `-verbose` print whole command line of executed compilers/linkers
* `-gcc_override=[compiler]` use specified compiler instead of `gcc`
* `-gxx_override=[compiler]` use specified compiler instead of `g++`
* `-clang_override=[compiler]` use specified compiler instead of `clang`
* `-clangxx_override=[compiler]` use specified compiler instead of `clang++`
* `-failexit` exit at first fail
* `-ignore_warnings` don't prompt, always continue if there are warnings
* `-num_jobs=[num_jobs]` execute `[num_jobs]` compilations in parallel
* `-version` display current version
* `-help` display help message
* `-filetime_nocache` don't cache file write times
* `-incremental_build_exclude_system` or `-MMD` to exclude system headers when generating dependency files

The layout for the json file is specified in [`info/json_layout.txt`](info/json_layout.txt), and the documentation for each field is at [`FORMAT.md`](FORMAT.md).

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
Requires MinGW
#### Option 1 (batch file)
* run `build.bat` to build it
* optionally manually add it to your path
#### Option 2 (codeblocks)
* open the `RBuild.cbp` project file in codeblocks, and compile it
* optionally manually add it to your path
#### Option 3 (itself)
* run `RBuild` on the project folder, if the project is in a folder named `RBuild`, or `RBuild -file=RBuild.json` if the project is in a different folder
* optionally manually add it to your path


## License

This project is licensed under the [MIT License](LICENSE).
