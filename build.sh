#!/bin/bash

# This script will download and compile the necessary libraries for the project
# Similar to bootstrap.sh but can be used as an alternative

set -e  # Exit on error

echo "Building dependencies for purplepois0n..."

# Create a build directory for dependencies
mkdir -p build-deps
cd build-deps

# Function to build a library
build_lib() {
    local repo=$1
    local name=$2
    
    echo "Building $name..."
    
    if [ -d "$name" ]; then
        echo "$name already exists, skipping clone..."
        cd "$name"
        git pull || true
    else
        git clone "$repo" "$name"
        cd "$name"
    fi
    
    if [ -f "autogen.sh" ]; then
        ./autogen.sh
    elif [ -f "configure" ]; then
        ./configure
    elif [ -f "CMakeLists.txt" ]; then
        mkdir -p build
        cd build
        cmake ..
        make
        sudo make install
        cd ..
        cd ..
        return
    fi
    
    make
    sudo make install
    cd ..
}

# Build dependencies
build_lib "https://github.com/libimobiledevice/libplist.git" "libplist"
build_lib "https://github.com/libimobiledevice/libusbmuxd.git" "libusbmuxd"
build_lib "https://github.com/libimobiledevice/libirecovery.git" "libirecovery"
build_lib "https://github.com/libimobiledevice/libimobiledevice.git" "libimobiledevice"

# Return to the parent directory
cd ..

echo "Dependencies built successfully!"
echo "You can now run 'make' to build purplepois0n"
