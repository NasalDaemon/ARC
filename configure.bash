#!/usr/bin/bash

BUILD_TYPE="Debug"
STD_MODULE="ON"
BUILD_EXAMPLES="ON"

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -c|--config)
            BUILD_TYPE="$2";
            shift ;;
        -m|--std-module)
            STD_MODULE="$2"
            ;;
        -e|--examples)
            BUILD_EXAMPLES="$2"
            ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

cd $(dirname "$0")

# sudo apt install cmake ninja-build mold python3 pipx
# pipx install conan
conan install . --output-folder=cmake-build --build=missing --profile=conanprofile.txt --settings=build_type=$BUILD_TYPE
cmake -B cmake-build --preset conan-default \
    -DARC_BUILD_TESTS=TRUE \
    -DARC_BUILD_EXAMPLES=$BUILD_EXAMPLES \
    -DARC_COMPRESS_TYPES=TRUE \
    -DCMAKE_COLOR_DIAGNOSTICS=TRUE \
    -DCMAKE_CXX_MODULE_STD=$STD_MODULE
