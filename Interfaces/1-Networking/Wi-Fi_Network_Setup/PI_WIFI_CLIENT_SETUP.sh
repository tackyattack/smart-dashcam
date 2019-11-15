#!/usr/bin/env bash

# Backup original wpa config file if backup doesn't exist
if [ ! -d OriginalBackups ]; then
    mkdir -p OriginalBackups
    sudo mv /etc/wpa_supplicant/wpa_supplicant.conf OriginalBackups/
fi

# Create new wpa config file. Modify the network settings below as needed
sudo bash -c 'echo "
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=US

network={
        ssid=\"DashcamWireless\"
        psk=\"dashcamwireless\"
}
" > /etc/wpa_supplicant/wpa_supplicant.conf'

# Reset wpa and reload configuration
sudo wpa_supplicant -B -iwlan0 -c/etc/wpa_supplicant/wpa_supplicant.conf
sudo wpa_cli -iwlan0 reconfigure
