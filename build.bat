@echo off
where /q RBuild.exe
IF ERRORLEVEL 1 (
    ECHO Cannot find RBuild in path, using fallback build commands

    ECHO building release...

    g++ src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -O2 -s -o build/win/release/bin/RBuild
    IF ERRORLEVEL 1 (
        ECHO Release build failed
    ) ELSE (
        ECHO Release build successful
    )
) ELSE (
    RBuild --file=RBuild.json release
)