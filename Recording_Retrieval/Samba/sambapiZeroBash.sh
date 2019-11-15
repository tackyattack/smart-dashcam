#!/usr/bin/env bash

# Install Samba
sudo apt-get update
sudo apt-get install samba samba-client samba-common -y

sudo mkdir -p Backups


# Make backup copy of the config file -> named smb_bkp.conf

if [ -f "Backups/smb_bkp.conf" ]
then
    echo "Backup samba config already exists!"
else
    sudo cp -f /etc/samba/smb.conf Backups/smb_bkp.conf
fi

# Bring in updated conf file
sudo cp -f samba_config_for_piZero.conf /etc/samba/smb.conf
# Start Samba
sudo service smbd restart
echo "Done with Samba Config and Mounting"
