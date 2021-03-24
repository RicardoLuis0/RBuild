#!/usr/bin/env bash
if command -v RBuild &> /dev/null

then
    RBuild --file=RBuild.json release
else
    
    echo Cannot find RBuild in path, using fallback build commands

    echo Building release...
    
    mkdir -p build/lin/release/bin

    if g++ src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -O2 -s -o build/lin/release/bin/RBuild
    then
        echo Release build successful
    else
        echo Release build failed
    fi
    
fi

echo run ./install.sh to add to path or upgrade

