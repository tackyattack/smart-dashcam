#!/usr/bin/env bash

# Install Proftpd
sudo apt-get update
sudo apt-get install proftpd

sudo mkdir -p Backups


# Make backup copy of the config file -> named proftpd_bkp.conf

if [ -f "Backups/proftpd_bkp.conf" ]
then
    echo "Backup proftpd config already exists!"
else
    sudo cp -f /etc/proftpd/proftpd.conf Backups/proftpd_bkp.conf
fi

# Bring in updated conf file
sudo cp FTPConfigPi3.conf /etc/proftpd/proftpd.conf

sudo service proftpd reload

# Start proftpd
sudo service proftpd start

echo "Done with FTP Config"
