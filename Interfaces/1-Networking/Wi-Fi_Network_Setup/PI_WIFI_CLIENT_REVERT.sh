#!/usr/bin/env bash

# Revert WPA to the backup copy of the original configuration if exists
if [ ! -f OriginalBackups/wpa_supplicant.conf ]; then
    exit 0
fi

sudo mv OriginalBackups/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf

# Reset wpa to reload configuration
sudo wpa_cli -iwlan0 reconfigure
