# Install Proftpd
sudo apt-get update
sudo apt-get install proftpd

# Start proftpd
sudo service proftpd start

# Make backup copy of the config file -> named smb_bkp.conf
sudo cp /etc/proftpd/proftpd.conf /etc/proftpd/proftpd_bkup.conf

# Remove original proftpd.conf
sudo rm /etc/proftpd/proftpd.conf

# Bring in updated conf file
sudo cp FTPConfigPi3.conf /etc/proftpd/proftpd.conf


sudo service proftpd reload

echo "Done with FTP Config"
