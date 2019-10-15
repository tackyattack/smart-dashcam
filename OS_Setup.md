
# Notes
  - Ethernet devices that are Davicom device (more specifically that use the DM9601 or DM96xx drivers) do not work on the Raspberry Pi's
  - If a network device is attached for which Linux does not have a driver, it seems that this screws with Linux such that it has random networking problems such as not being able to change Wi-Fi networks.
  - It seems easiest to change Wi-Fi networks by altering the /etc/wpa_supplicant/wpa_supplicant.conf file such that the network you want to connect to is listed first followed by rebooting the system


# Raspbian Setup Instructions for Pi Zero W

1. Burn the [Raspbian Buster Lite](https://www.raspberrypi.org/downloads/raspbian/) image to a microSD card
2. Boot RPi and log in with default user: *pi* and password: *raspberry*
3. Change the password to *dashcam2019* via `passwd pi`
4. Using `sudo raspi-config`:
   * [development only]  Enable SSH
   * Set hostname to *dashcam_aux0* where the '0' is replaced with a device number ('0' should be for the pi zero W with the backup camera connected)
   * Add Wi-Fi network with SSID *DashcamWireless* and password *dashcamwireless*
5. [development only] Type `sudo nano /etc/ssh/sshd_config`
   * Modify the line that specifies the port to use. Each RPi/aux unit should have a different port number.
   * Add the line `PasswordAuthentication yes`
   * Add the line `PermitRootLogin yes`
6. Reboot system
7. Install dependencies (add any needed software packages to the list below): copy and paste the command below:
   ```sh
   sudo apt install libdbus-1-dev libdbus-glib-1-dev
   ```
8. ```sh
   pip install PySimpleGUI27
   pip install typing
   ```

 # Raspbian Setup Instructions for RPi 3
 Need to install samba to fix hostname lookups used by the TCP comm code (Networking/comm_tcp_interface). Alternatively, disable IPv6 on the hosted network
```sh
sudo bash -c 'echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf'
sysctl -p
```
and use an ip address instead of hostname in the Networking/comm_tcp_interface/clients/comm_tcp_client.c in the SERVER_ADDR #define.

 # Alpine Linux Setup Instructions for RPi Zero W
 1. Disable IPV6
   ```sh
echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf
sysctl -p
```
You may need to run the setup-interfaces script and reboot. Warning, you must run the script in its entirety.
Need to test to see if disabling ipv6 is necessary anymore. If so, should disable it on all interfaces.
2. Need to install glib, git, cmake, and gcc for compiling code
