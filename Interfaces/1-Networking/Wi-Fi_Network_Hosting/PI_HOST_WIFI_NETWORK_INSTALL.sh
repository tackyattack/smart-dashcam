#!/bin/bash

#install dependencies
sudo apt install dnsmasq hostapd -y

#stop services
sudo systemctl stop dnsmasq
sudo systemctl stop hostapd
sudo systemctl disable dnsmasq
sudo systemctl disable hostapd

#This is needed for some reason relating to recent updates to hostapd
sudo systemctl unmask hostapd

#Backup files that will be modified
sudo cp -f /etc/dhcpcd.conf             /etc/dhcpcd.conf_original
sudo cp -f /etc/dnsmasq.conf            /etc/dnsmasq.conf_original
sudo cp -f /etc/hostapd/hostapd.conf    /etc/hostapd/hostapd.conf_original
sudo cp -f /etc/default/hostapd         /etc/default/hostapd_original

#backup files to local folder as well
mkdir   -p backups
sudo cp -f /etc/dhcpcd.conf            backups/dhcpcd.conf
sudo cp -f /etc/dnsmasq.conf           backups/dnsmasq.conf
sudo cp -f /etc/hostapd/hostapd.conf   backups/hostapd.conf
sudo cp -f /etc/default/hostapd        backups/hostapd

#Create working files that we use to store the config we wish to use
mkdir   -p WorkingConfigs
sudo cp -f backups/dhcpcd.conf     WorkingConfigs/dhcpcd.conf
sudo cp -f backups/dnsmasq.conf    WorkingConfigs/dnsmasq.conf
sudo cp -f backups/hostapd.conf    WorkingConfigs/hostapd.conf
sudo cp -f backups/hostapd         WorkingConfigs/hostapd


######### NETWORK CONFIG #########

#Append the following to our working copy of /etc/dhcpcd.conf:
sudo bash -c 'sudo echo "
interface wlan0
    static ip_address=192.168.200.1/24
    static routers=192.168.200.1
    static domain_name_servers=192.168.200.1
    nohook wpa_supplicant
" >> WorkingConfigs/dhcpcd.conf'

#Append the following to our working copy of /etc/dnsmasq.conf:
sudo bash -c 'echo "
interface=wlan0 
 dhcp-range=wlan0,192.168.200.2,192.168.200.10,255.255.255.0,24h" >> WorkingConfigs/dnsmasq.conf'

#Append the following to  our working copy of /etc/hostapd/hostapd.conf
sudo bash -c 'echo "interface=wlan0
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
" >> WorkingConfigs/hostapd.conf'

#Append/modify the following to our working copy of /etc/default/hostapd
sudo bash -c 'echo "DAEMON_CONF=\"/etc/hostapd/hostapd.conf\"" >> WorkingConfigs/hostapd'

echo "
Finished Wi-Fi Host Mode Installation!"
