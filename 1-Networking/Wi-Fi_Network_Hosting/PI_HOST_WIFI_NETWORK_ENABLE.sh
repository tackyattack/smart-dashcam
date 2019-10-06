#!/bin/bash

#Copy configs to appropiate directories
sudo cp -f WorkingConfigs/dhcpcd.conf       /etc/dhcpcd.conf     
sudo cp -f WorkingConfigs/dnsmasq.conf      /etc/dnsmasq.conf     
sudo cp -f WorkingConfigs/hostapd.conf      /etc/hostapd/hostapd.conf
sudo cp -f WorkingConfigs/hostapd           /etc/default/hostapd     

sudo bash -c 'systemctl daemon-reload'
sudo bash -c 'service dhcpcd restart'
sudo bash -c 'systemctl enable hostapd'
sudo bash -c 'systemctl enable dnsmasq'

sudo systemctl daemon-reload
sudo service dhcpcd restart
sudo service dnsmasq restart
sudo service hostapd restart

echo "
Activated Wi-Fi Host Mode: Note, it is recommended to reboot."
