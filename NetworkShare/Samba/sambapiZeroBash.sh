# Install Samba
sudo apt-get update
sudo apt-get install samba samba-client samba-common

# Make backup copy of the config file -> named smb_bkp.conf
sudo cp -f /etc/samba/smb.conf /etc/samba/smb_bkp.conf

# Bring in updated conf file
sudo cp -f samba_config_for_piZero.conf /etc/samba/smb.conf

# Start Samba
sudo service smbd restart

echo "Done with Samba Config and Mounting"
