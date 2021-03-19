#!/usr/bin/env bash
if command -v RBuild &> /dev/null
then
    RBuild --file=RBuild.json
else
    echo Cannot find RBuild in path, using fallback build commands
    
    echo Building debug...
    
    if g++ src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -g -o build/lin/debug/bin/RBuild
    then
        echo Debug build successful
    else
        echo Debug build failed
    fi

    echo Building release...

    if g++ src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -O2 -s -o build/lin/release/bin/RBuild
    then
        echo Release build successful
    else
        echo Release build failed
    fi
fi

echo run ./install.sh to add to path or upgrade

