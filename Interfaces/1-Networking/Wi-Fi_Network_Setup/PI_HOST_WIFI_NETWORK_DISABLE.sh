#!/usr/bin/env bash

#Disable/stop services
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq
sudo systemctl disable hostapd
sudo systemctl disable dnsmasq

#Restore backup copies of network configs
sudo cp -f OriginalBackups/dhcpcd.conf      /etc/dhcpcd.conf     
sudo cp -f OriginalBackups/dnsmasq.conf     /etc/dnsmasq.conf     
sudo cp -f OriginalBackups/hostapd.conf     /etc/hostapd/hostapd.conf
sudo cp -f OriginalBackups/hostapd          /etc/default/hostapd   

#Restart services
sudo systemctl daemon-reload
sudo service dhcpcd restart

# Reset wpa to reload configuration
sudo wpa_cli -iwlan0 reconfigure

echo "
Deactivated Wi-Fi Host Mode: It is recommended to reboot."