#!/usr/bin/env bash

# Install dependencies
sudo apt install dnsmasq hostapd -y

# Stop services
sudo systemctl stop dnsmasq
sudo systemctl stop hostapd
sudo systemctl disable dnsmasq
sudo systemctl disable hostapd

# This is needed for some reason relating to recent updates to hostapd
sudo systemctl unmask hostapd

# Check if backups folder exists. If it does, this script has been run previously
#   and that means we don't want to overwrite the original copies that were made
if [ ! -d OriginalBackups ]; then
    # Backup files that will be modified to local folder
    mkdir   -p OriginalBackups
    sudo cp -f /etc/dhcpcd.conf            OriginalBackups/dhcpcd.conf
    sudo cp -f /etc/dnsmasq.conf           OriginalBackups/dnsmasq.conf
    sudo cp -f /etc/hostapd/hostapd.conf   OriginalBackups/hostapd.conf
    sudo cp -f /etc/default/hostapd        OriginalBackups/hostapd
else
    echo "\nWi-Fi Network Installation detected that configurations are already installed. Overwriting them from local OriginalBackups...\n"
fi

# Create working files that we use to store the config we wish to use
mkdir   -p GeneratedConfigs
sudo cp -f OriginalBackups/dhcpcd.conf     GeneratedConfigs/dhcpcd.conf
sudo cp -f OriginalBackups/dnsmasq.conf    GeneratedConfigs/dnsmasq.conf
sudo cp -f OriginalBackups/hostapd.conf    GeneratedConfigs/hostapd.conf
sudo cp -f OriginalBackups/hostapd         GeneratedConfigs/hostapd


######### NETWORK CONFIGS #########

# Append the following to our working copy of /etc/dhcpcd.conf:
sudo bash -c 'sudo echo "
interface wlan0
    static ip_address=192.168.200.1/24
    static routers=192.168.200.1
    static domain_name_servers=192.168.200.1
    nohook wpa_supplicant
" >> GeneratedConfigs/dhcpcd.conf'

# Append the following to our working copy of /etc/dnsmasq.conf:
sudo bash -c 'echo "
interface=wlan0 
 dhcp-range=wlan0,192.168.200.2,192.168.200.10,255.255.255.0,24h" >> GeneratedConfigs/dnsmasq.conf'

# Append the following to  our working copy of /etc/hostapd/hostapd.conf
sudo bash -c 'echo "
interface=wlan0
ssid=DashcamWireless
wpa_passphrase=dashcamwireless
ignore_broadcast_ssid=0
#driver=nl80211 #shouldnt need to specify
hw_mode=g
#hw_mode=a #sets wireless to be 5GHz instead. Not supported by PI
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
" >> GeneratedConfigs/hostapd.conf'

# Append/modify the following to our working copy of /etc/default/hostapd
sudo bash -c 'echo "DAEMON_CONF=\"/etc/hostapd/hostapd.conf\"" >> GeneratedConfigs/hostapd'

echo "
Finished Wi-Fi Host Mode Installation!\n"
