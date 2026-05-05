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

set -e
cmake --build cmake-build --config $BUILD_TYPE
ctest --test-dir cmake-build --build-config $BUILD_TYPE --output-on-failure
