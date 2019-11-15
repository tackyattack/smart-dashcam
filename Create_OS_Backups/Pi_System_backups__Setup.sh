#!/usr/bin/env bash

# Instructions and resources/sources
# https://github.com/aktos-io/dcs-tools

# Dependencies
sudo apt update
sudo apt install git rsync sshfs

# Get source code
mkdir -p tools
cd tools
git clone --recursive https://github.com/aktos-io/dcs-tools

./dcs-tools/setup

echo "Test SSH connection for success..."
make ssh
