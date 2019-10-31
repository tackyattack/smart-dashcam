# Install Samba
sudo apt-get update
sudo apt-get install samba

# Start Samba
sudo service smbd start

# Make backup copy of the config file -> named smb_bkp.conf
sudo cp /etc/samba/smb.conf /etc/samba/smb_bkp.conf

# Remove original smb.conf
sudo rm /etc/samba/smb.conf

# Bring in updated conf file
sudo cp samba_config_for_piZero.conf /etc/samba/smb.conf

# Mount Shared piZero Drive
sudo mkdir recordings
sudo mount -t cifs //192.168.200.2/sambashare ~/recordings

echo "Done with Samba Config and Mounting"
