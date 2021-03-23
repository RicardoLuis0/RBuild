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

## Using

The command-line arguments the program accepts are:
* `-file=[json file]` to specify which file it uses as the project file, if no file is specified it uses "[current folder name].json"
* `-rebuild` rebuild project from scratch, ignore already-built object files
* `-verbose` print whole command line of executed compilers/linkers

The format for the json file is specified in [`info/json_layout.txt`](info/json_layout.txt)

### Project Properties

#### Compilers

The project's compilers can be specified via the `compiler_all`, `compiler_c_cpp`, `compiler_c`, `compiler_cpp` and `compiler_asm` properties, with more specific flags taking precedence.

Currently supported compilers are:
* GCC/G++ for C/C++/ASM with `gcc`
* clang/clang++ for C/C++/ASM with `clang`
* GAS for ASM with `as`
* NASM for ASM with `nasm`

#### Linkers

The project's linker can be specified via the `linker` property.

Currently supported linkers are:
* GCC/G++ with `gcc`
* clang/clang++ with `clang`
* ld with `ld`
* ld.gold with `ld.gold`
* ld.lld with `ld.lld`
* ar with `ar`
* llvm-ar with `llvm-ar`

#### Misc Project Properties

`project_name`: The name that will be shown when building  
`working_folder`: **REQUIRED** The folder where the build files will be stored.  
`project_binary`: **REQUIRED** Output binary for the project, without extension  
`binary_folder_override`: Override output folder for binary (from `working_folder\arch\target\bin`) to this  
`project_ext`: Override extension (from `.exe` on windows and empty on linux) to this  
`src_folder`: **REUQIRED only if sources are in a different folder than cwd**  
`noarch`: if true don't separate built files per architecture, default false  
`targets_default`: default target or default targets, the targets that will be built if no targets are specified in the command-line, default `all`

### Target Properties

#### Specifying Source Files/Folders

A source file/folder can be either a string or a source object. If is an object, the properties are:
* `name`: file/folder
* `mode`: ignored if file, can be `exclude`(blacklist) or `include`(whitelist).
* `exclude_list`: array of strings (files/folders) to be ignored
* `include_list` array of sources (files/folders) to be included

#### Including Other Targets

The target will inherit the properties of any other targets specified in the `includes` property.

#### Compiler Properties

The `flags_*` properties speficy the flags to be passed to the compilers, each string in the array will be passed as a single argument, no manual escaping is necessary.

The available properties are  `flags_all`, `flags_c_cpp`, `flags_c`, `flags_cpp` and `flags_asm`. All applicable properties will be passed to the compilers, for example, the C compiler will receive all flags from `flags_all`, `flags_c_cpp` and `flags_c`.

The `defines_*` properties speficy the defines to be passed to the compilers, the defines will be converted to the format the compiler accepts, for example, a `defines_all` of `DEBUG` will be passed to the C compiler as `-DDEBUG` and to GAS as `--defsym DEBUG=1`.

Same as with flags, the available properties are  `defines_all`, `defines_c_cpp`, `defines_c`, `defines_cpp` and `defines_asm`.

#### Linker Properties

The `linker_flags` property speficies the flags to be passed to the linker, each string in the array will be passed as a single argument, no manual escaping is necessary.

The `linker_libs` property speficies the libraries to be passed to the linker, will be passed as-is, like `linker_flags`.

The `linker_order` property specifies exceptions to the default linking order, positive weights means it will be linked after all other files, and negative weights means it will be linked before all other files.

It has 3 types:
* `normal` (the default) will try to match only the filename of the object file;
* `full_path` will try to match the full path (ex. if you want to link `src/foo/foo.cpp` before all else you'd write `foo.cpp.o` as the name if `normal`, and `src/foo/foo.cpp.o` if `full_path`)
* `extra` just appends the specified file path to the object file list.

#### Misc Target Properties

`include_only`: will not allow the target to be compiled by itself, only included by other targets

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
