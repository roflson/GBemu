#!/bin/bash

# Change to the build directory (create if it doesn't exist)
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 1

# Run cmake to generate compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cp compile_commands.json ..

echo "compile_commands.json generated in $BUILD_DIR/"
