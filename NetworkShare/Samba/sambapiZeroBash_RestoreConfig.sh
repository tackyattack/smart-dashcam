if [ -f "Backups/smb_bkp.conf" ]
then
    sudo cp -f Backups/smb_bkp.conf /etc/samba/smb.conf
    echo "Replaced Samba Config with original file"
    # Remove Backup Folder
    sudo rm -rf Backups
else
    echo "samba backup file does NOT exist!"
fi
# Restart Samba
sudo service smbd restart
echo "Restored original Samba Config"
