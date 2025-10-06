#!/usr/bin/bash

cd $(dirname "$0")

BUILD_TYPE="Release"
STD_MODULE="OFF"

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -c|--config)
            BUILD_TYPE="$2";
            shift ;;
        -m|--std-module)
            STD_MODULE="ON"
            ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

# sudo apt install cmake ninja-build mold python3 pipx
# pipx install conan
conan install . --output-folder=build --build=missing --profile=conanprofile.txt --settings=build_type=$BUILD_TYPE
cd build
cmake .. --preset conan-default \
    -DARC_BUILD_TESTS=TRUE \
    -DARC_COMPRESS_TYPES=TRUE \
    -DCMAKE_COLOR_DIAGNOSTICS=TRUE \
    -DCMAKE_CXX_MODULE_STD=$STD_MODULE
cmake --build . --config $BUILD_TYPE
ctest --build-config $BUILD_TYPE --output-on-failure
