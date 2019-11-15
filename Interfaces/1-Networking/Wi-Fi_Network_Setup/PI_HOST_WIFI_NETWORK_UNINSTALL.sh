#!/usr/bin/env bash

# Stop/disable services
sudo systemctl stop     hostapd
sudo systemctl stop     dnsmasq
sudo systemctl disable  hostapd
sudo systemctl disable  dnsmasq

# Restore original copies of network config files/delete any that didn't previously exist
sudo cp -f OriginalBackups/dhcpcd.conf      /etc/dhcpcd.conf
sudo cp -f OriginalBackups/dnsmasq.conf     /etc/dnsmasq.conf
sudo cp -f OriginalBackups/hostapd.conf     /etc/hostapd/hostapd.conf
sudo cp -f OriginalBackups/hostapd          /etc/default/hostapd
sudo rm -f /etc/default/hostapd #/etc/hostapd/hostapd.conf /etc/dnsmasq.conf

#Delete the backup and working files
sudo rm -fr OriginalBackups GeneratedConfigs

# Purge dnsmasq and hostapd which were installed in the setup script
sudo dpkg --force-all --purge dnsmasq hostapd # Ensure config files are erased
sudo apt autoclean -y
sudo apt autoremove -y

# Reload the services
sudo systemctl daemon-reload
sudo service dhcpcd restart

# Reset wpa to reload configuration
sudo wpa_cli -iwlan0 reconfigure

#restart computer
#sudo reboot

echo "
Finished removing Wi-Fi Host Mode. It is recommended to reboot.\n"
