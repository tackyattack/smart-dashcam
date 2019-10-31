# Mount Shared piZero Drive
IP="$1"
dir="$2"

if [ -d "/Recordings/$dir" ]
then
    echo "Failed to Mount, directory already exists!"
else
    sudo mkdir -p /Recordings/$dir
    
sudo mount -t cifs //$IP/sambashare /Recordings/$dir

echo "Mounted $dir Drive"
fi
