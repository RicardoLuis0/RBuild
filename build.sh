#!/usr/bin/env bash
gcc_version=$(gcc --version | awk '/gcc/{print $NF}')

if [ ${gcc_version%%.*} -lt 10 ]
then
    if command -v gcc-10 &> /dev/null
    then
        if command -v g++-10 &> /dev/null
        then
            use_gcc_10=1
        else
            echo DEFAULT GCC TOO OLD, FOUND GCC-10, MISSING G++-10
            exit 1
        fi
    elif command -v g++-10 &> /dev/null
    then
        echo DEFAULT GCC TOO OLD, MISSING GCC-10, FOUND G++-10
        exit 1
    else
        echo DEFAULT GCC TOO OLD, MISSING GCC-10, MISSING G++-10
        exit 1
    fi
else
    use_gcc_10=0
fi

if command -v RBuild &> /dev/null
then
    
    if [ $use_gcc_10 == 1 ]
    then
        RBuild --gcc_override=gcc-10 --gxx_override=g++-10 --file=RBuild.json release
    else
        RBuild --file=RBuild.json release
    fi
    
else
    
    if [ $use_gcc_10 == 1 ]
    then
        gxx="g++-10"
    else
        gxx="g++"
    fi
    
    echo Cannot find RBuild in path, using fallback build command

    echo Building release...
    
    mkdir -p build/lin/release/bin
    if $gxx src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -O2 -s -o build/lin/release/bin/RBuild
    then
        echo Release build successful
    else
        echo Release build failed
    fi
    
fi

echo run ./install.sh to add to path or upgrade

