#!/usr/bin/env bash

# https://github.com/aktos-io/dcs-tools

make conn-direct # Make connection to remote system
# make ssh         # Setup ssh'ing into remote system
make mount-root  # Mount the remote file system
make sync-root   # Copy remote file system to local folder
make backup-sync-root # Create the backup of the local copy of the remote file system
make make umount-root
