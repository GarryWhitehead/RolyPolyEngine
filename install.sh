#!/usr/bin/env bash

set -E
trap 'catch $? $LINENO' ERR

catch(){
    if [ "$1" != 0 ]; then
        echo "Error $1 occured on line $2"
        exit $1
    fi
}

echo """
Welcome to the RolyPoly Engine installation script.
This script will set up your system ready for use with the YAVE engine.

Setting up conan....

"""

# Check that conan is installed on the system
conan_found=$(python -m pip list | grep -i -c conan) || true
if [ $conan_found -eq 1 ]; then
    echo -e "\nFound conan on system."
else
    echo -e "\nYou need to pip install conan before running this script and ensure the PATH is set."
fi

# Build the packages that are not found on the Conan center or have issues with our buildystem (dependency clashes, etc.)
conan_recipe_path="conan/recipes"
if [ ! -d $conan_recipe_path ]; then
    echo -e "\nNo Conan recipe folder found at $conan_recipe_path. Skipping building additional conan packages."
else
    echo -e "\nBuilding package: libshaderc...."
    conan create $conan_recipe_path/shaderc/all 2021.1@ -pr:b=default -pr:h=default --build=missing
fi

# Download the vulkan assets package
echo -e "\nDownloading Vulkan sample assets from git repo...."

git_https="https://github.com/KhronosGroup/Vulkan-Samples-Assets.git"
assets_folder="assets"

if [ -d $assets_folder ]; then
    echo -e "\nAssets directory already exists. Skipping download."
else
    git clone $git_https
    mv "Vulkan-Samples-Assets" $assets_folder
    echo -e "\nVulkan assets pack successfully downloaded to $assets_folder"
fi

echo """

Installation for RPE completed successfully.
"""



