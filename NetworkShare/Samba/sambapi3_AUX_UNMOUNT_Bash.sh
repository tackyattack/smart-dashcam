# UNMount Shared piZero Drive
dir="$1"

if [ -d "/Recordings/$dir" ]
then
    sudo umount /Recordings/$dir
    sudo rm -rf /Recordings/$dir
    echo "Un-Mounted Sucessfully!"
else
    echo "Directory Does NOT Exist!"
fi
