# JSON Format

## Project Properties

### Compilers

The project's compilers can be specified via the `compiler_all`, `compiler_c_cpp`, `compiler_c`, `compiler_cpp` and `compiler_asm` properties, with more specific flags taking precedence.

Currently supported compilers are:
* GCC/G++ for C/C++/ASM with `gcc`
* clang/clang++ for C/C++/ASM with `clang`
* GAS for ASM with `as`
* NASM for ASM with `nasm`

### Linkers

The project's linker can be specified via the `linker` property.

Currently supported linkers are:
* GCC/G++ with `gcc`
* clang/clang++ with `clang`
* ld with `ld`
* ld.gold with `ld.gold`
* ld.lld with `ld.lld`
* ar with `ar`
* llvm-ar with `llvm-ar`

### Misc Project Properties

* `project_name`: The name that will be shown when building
* `targets`: List of the targets for the project
* `working_folder`: **REQUIRED** The folder where the build files will be stored.
* `project_binary`: **REQUIRED** Output binary for the project, without extension
* `binary_folder_override`: Override output folder for binary (from `working_folder\arch\target\bin`) to this
* `project_ext`: Override extension (from `.exe` on windows and empty on linux) to this
* `src_folder`: **REUQIRED only if sources are in a different folder than cwd**
* `noarch`: if true don't separate built files per architecture, default false
* `targets_default`: default target or default targets, the targets that will be built if no targets are specified in the command-line, default `all`
* `compiler_binary_override_c`,`compiler_binary_override_cpp`,`compiler_binary_override_c_cpp`,`compiler_binary_override_asm`,`compiler_binary_override_all`: change the binary that is executed when calling the compiler
* `linker_binary_override_c`,`linker_binary_override_cpp`,`linker_binary_override_c_cpp`,`linker_binary_override_other`,`linker_binary_override_all`: change the binary that is executed when calling the linker

## Target Properties

### Specifying Source Files/Folders

A source file/folder can be either a string or a source object. If is an object, the properties are:
* `name`: file/folder
* `type`: ignored if file, can be `exclude` (or `blacklist`), `include` (or `whitelist`), `include_folders_exclude_files` (or `folder_whitelist_file_blacklist`/`exclude_files_include_folders`/`file_blacklist_folder_whitelist`).
* `exclude_list` or `blacklist`: array of strings (files/folders) to be ignored
* `include_list` or `whitelist`: array of sources (files/folders) to be included

### Including Other Targets

The target will inherit the properties of any other targets specified in the `includes` property.

#### Include Switches

To specify an include switch, a JSON Object must be used as an include target, instead of a string.

It must have a `type` property with the value of `switch`, a valid `condition` property, and a `cases` property, which defines which specific target to include based on the value of the condition.

Supported conditions are:
* `platform`: The platform that the target is being built for.

Example:
```json
"include":[
    "common_all",
    {
        "type":"switch",
        "condition":"platform",
        "cases":{
            "linux":"common_linux",
            "windows":"common_windows"
        }
    }
]
```

### Grouping Targets

A target group can be specified instead of a target in `targets` by using the `target_group` property, which is a list of the names of targets/other target groups which are included by the group.

Example:
```json
"targets":[
    "my_target_01":{
    },
    "my_target_02":{
    },
    "my_target_group":{
        "target_group":[
            "my_target_01",
            "my_target_02",
        ]
    }
]
```

### Compiler Properties

The `flags_*` properties speficy the flags to be passed to the compilers, each string in the array will be passed as a single argument, no manual escaping is necessary.

The available properties are  `flags_all`, `flags_c_cpp`, `flags_c`, `flags_cpp` and `flags_asm`. All applicable properties will be passed to the compilers, for example, the C compiler will receive all flags from `flags_all`, `flags_c_cpp` and `flags_c`.

The `defines_*` properties speficy the defines to be passed to the compilers, the defines will be converted to the format the compiler accepts, for example, a `defines_all` of `DEBUG` will be passed to the C compiler as `-DDEBUG` and to GAS as `--defsym DEBUG=1`.

Same as with flags, the available properties are  `defines_all`, `defines_c_cpp`, `defines_c`, `defines_cpp` and `defines_asm`.

### Linker Properties

The `linker_flags` property speficies the flags to be passed to the linker, each string in the array will be passed as a single argument, no manual escaping is necessary.

The `linker_libs` property speficies the libraries to be passed to the linker, will be passed as-is, like `linker_flags`.

The `linker_order` property specifies exceptions to the default linking order, positive weights means it will be linked after all other files, and negative weights means it will be linked before all other files.

It has 3 types:
* `normal` (the default) will try to match only the filename of the object file;
* `full_path` will try to match the full path (ex. if you want to link `src/foo/foo.cpp` before all else you'd write `foo.cpp.o` as the name if `normal`, and `src/foo/foo.cpp.o` if `full_path`)
* `extra` just appends the specified file path to the object file list.

### Misc Target Properties

* `include_only`: will not allow the target to be compiled by itself, only included by other targets  
* `binary_folder_override`: override output folder for binary (from `working_folder\arch\target\bin`) to this for target  
* `project_binary_override`: override `project_binary` property for the target
* `target_folder_override`: override the folder where the target's output files (obj/bin/etc) will be stored, if the folder is already another target, don't forget to specify `project_binary_override` to prevent overwriting the output executable
* `compiler_driver_override_c`,`compiler_driver_override_cpp`,`compiler_driver_override_asm`,`compiler_driver_override_all`,`linker_driver_override`: override project's compiler/linker properties
* `compiler_binary_override_c`,`compiler_binary_override_cpp`,`compiler_binary_override_c_cpp`,`compiler_binary_override_asm`,`compiler_binary_override_all`: change the binary that is executed when calling the compiler
* `linker_binary_override_c`,`linker_binary_override_cpp`,`linker_binary_override_c_cpp`,`linker_binary_override_other`,`linker_binary_override_all`: change the binary that is executed when calling the linker
