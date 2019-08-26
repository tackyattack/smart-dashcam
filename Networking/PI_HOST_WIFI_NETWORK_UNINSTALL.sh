#!/bin/bash

#Stop/disable services
sudo systemctl stop     hostapd
sudo systemctl stop     dnsmasq
sudo systemctl disable  hostapd
sudo systemctl disable  dnsmasq

#Restore original copies of network config files/delete any that didn't previously exist
sudo cp -f backups/dhcpcd.conf                   /etc/dhcpcd.conf     
#sudo cp -f /etc/dnsmasq.conf_original           /etc/dnsmasq.conf     
#sudo cp -f /etc/hostapd/hostapd.conf_original   /etc/hostapd/hostapd.conf
#sudo cp -f /etc/hostapd_original                /etc/default/hostapd     
sudo rm -f  /etc/hostapd/hostapd.conf /etc/default/hostapd /etc/dnsmasq.conf

#Delete the backup and working files
sudo rm -f /etc/dhcpcd.conf_original
sudo rm -f /etc/dnsmasq.conf_original 
sudo rm -f /etc/hostapd/hostapd.conf_original
sudo rm -f /etc/hostapd_original
sudo rm -fr backups WorkingConfigs

#purge dnsmasq and hostapd which were installed in the setup script
sudo apt purge dnsmasq hostapd -y
sudo apt autoclean -y
sudo apt autoremove -y

#Reload the services
sudo systemctl daemon-reload
sudo service dhcpcd restart

#restart computer
#sudo reboot

echo "
Finished removing Wi-Fi Host Mode. It is recommended to reboot."
