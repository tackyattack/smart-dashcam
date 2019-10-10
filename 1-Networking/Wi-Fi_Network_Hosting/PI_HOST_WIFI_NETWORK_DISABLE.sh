#!/bin/bash

#Disable/stop services
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq
sudo systemctl disable hostapd
sudo systemctl disable dnsmasq

#Restore backup copies of network configs
sudo cp -f backups/dhcpcd.conf      /etc/dhcpcd.conf     
sudo cp -f backups/dnsmasq.conf     /etc/dnsmasq.conf     
sudo cp -f backups/hostapd.conf     /etc/hostapd/hostapd.conf
sudo cp -f backups/hostapd          /etc/default/hostapd   

#Restart services
sudo systemctl daemon-reload
sudo service dhcpcd restart

echo "
Deactivated Wi-Fi Host Mode: It is recommended to reboot."