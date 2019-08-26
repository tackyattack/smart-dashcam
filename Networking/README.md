# Raspberry Pi Wi-Fi Network Hosting Scripts

These scripts are used to setup a Wi-Fi network hosted by a Raspberry Pi. At the time this was written, rPi Zero W and rPi 3 B+ are tested to work with these scripts.

Start by configuring the Wi-Fi network, then install and enable the network to host the Wi-Fi network. Note that you must configure the network before running the setup/install script.

## Configuration
To configure the Wi-Fi network settings such as ssid, password, ip range, etc, modify the PI_HOST_WIFI_NETWORK_INSTALL.sh script in the NETWORK CONFIG section. Any changes made here will require reinstallation to reflect the changes.

## Installation
Execute PI_HOST_WIFI_NETWORK_INSTALL.sh to install necessary software and setup the rPi. By default the rPi will start not hosting the network. Please note that errors/warnings regarding hostapd and stat warnings about hostapd/dnsmasq files.

## Enable Hosted Wi-Fi Network
Execute the PI_HOST_WIFI_NETWORK_ENABLE.sh script to host the Wi-Fi network.

## Disable Hosted Wi-Fi Network
Execute the PI_HOST_WIFI_NETWORK_DISABLE.sh script to revert back to non Wi-Fi host mode to allow connecting to Wi-Fi networks.

## Uninstall/Removal
To disable and remove all aspects of the Wi-Fi hosting setup, simply execute PI_HOST_WIFI_NETWORK_DISABLE.sh followed by PI_HOST_WIFI_NETWORK_UNINSTALL.sh.