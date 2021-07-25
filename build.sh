#!/usr/bin/env bash

set -eu -o pipefail

function find_compatible_gxx_ver(){
    min_req=$1
    default_gcc_version=$(g++ --version | awk '/g\+\+/{print $NF}')
    if [ ${default_gcc_version%%.*} -lt $min_req ]; then
        avail_vers=($(compgen -c g++- | grep -oP "(?<=g\+\+-)[0-9]+"))
        if [ ${#avail_vers[@]} -gt 0 ]; then
            max=${avail_vers[0]}
            for ver in ${avail_vers[@]}
            do
                if [ $ver -gt $max ]; then
                    max=$ver
                fi
            done
            if [ $max -ge $min_req ]; then
                echo "g++-$max"
            else
                echo "NO VALID G++ VERSION FOUND, MINIMUM REQUIRED: $min_req , MAXIMUM FOUND: $max"
                return 1
            fi
        else
            echo "NO G++ VERSIONS FOUND"
            return 1
        fi
    else
        echo "g++"
    fi
}

function find_compatible_gcc_ver(){
    min_req=$1
    default_gcc_version=$(gcc --version | awk '/gcc/{print $NF}')
    if [ ${default_gcc_version%%.*} -lt $min_req ]; then
        avail_vers=($(compgen -c gcc- | grep -oP "(?<=gcc-)[0-9]+"))
        if [ ${#avail_vers[@]} -gt 0 ]; then
            max=${avail_vers[0]}
            for ver in ${avail_vers[@]}
            do
                if [ $ver -gt $max ]; then
                    max=$ver
                fi
            done
            if [ $max -ge $min_req ]; then
                echo "gcc-$max"
            else
                echo "NO VALID GCC VERSION FOUND, MINIMUM REQUIRED: $min_req , MAXIMUM FOUND: $max"
                return 1
            fi
        else
            echo "NO GCC VERSIONS FOUND"
            return 1
        fi
    else
        echo "gcc"
    fi
}

if gcc=$(find_compatible_gcc_ver 10)
then
    if gxx=$(find_compatible_gxx_ver 10)
    then
        :
    else
        echo "$gxx"
        exit 1
    fi
else
    echo "$gcc"
    exit 1
fi

if command -v RBuild &> /dev/null
then
    RBuild --gcc_override=$gcc --gxx_override=$gxx --file=RBuild.json "$@"
elif command -v ./build/lin/release/bin/RBuild &> /dev/null
then
    ./build/lin/release/bin/RBuild --gcc_override=$gcc --gxx_override=$gxx --file=RBuild.json "$@"
else
    echo Cannot find RBuild in path, using fallback build command

    echo Building release...
    
    mkdir -p build/lin/release/bin
    if $gxx src/args.cpp src/json.cpp src/main.cpp src/project.cpp src/targets.cpp src/util.cpp src/drivers.cpp -Iinclude -Werror=return-type -Werror=suggest-override -std=c++20 -fexceptions -O2 -lpthread -s -o build/lin/release/bin/RBuild
    then
        echo Release build successful
    else
        echo Release build failed
    fi
fi

echo run ./install.sh to add to path or upgrade

