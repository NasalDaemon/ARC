#!/usr/bin/bash

cd $(dirname "$0")

BUILD_TYPE="Debug"

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -c|--config)
            BUILD_TYPE="$2";
            shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

cd build

set -e
cmake --build . --config $BUILD_TYPE
ctest --build-config $BUILD_TYPE --output-on-failure
