
# Notes
  - Ethernet devices that are Davicom device (more specifically that use the DM9601 or DM96xx drivers) do not work on the Raspberry Pi's
  - If a network device is attached for which Linux does not have a driver, it seems that this screws with Linux such that it has random networking problems such as not being able to change Wi-Fi networks.
  - It seems easiest to change Wi-Fi networks by altering the /etc/wpa_supplicant/wpa_supplicant.conf file such that the network you want to connect to is listed first followed by rebooting the system
  - You can't use a hostname as the server address in the socket client library due to RPi OS bug that is out of my control or ability to fix.

# Raspbian Setup Instructions for Pi Zero W

1. Burn the [Raspbian Buster Lite](https://www.raspberrypi.org/downloads/raspbian/) image to a microSD card
2. Boot RPi and log in with default user: *pi* and password: *raspberry*
3. Change the password to *dashcam2019* via `passwd pi`
4. Using `sudo raspi-config`:
   * [development only]  Enable SSH
   * [optional] Set hostname to *dashcam_aux0* where the '0' is replaced with a device number that should be unique but in reality this doesn't matter as hostnames are broken on Raspbian.
5. [development only] Type `sudo nano /etc/ssh/sshd_config`
   * Modify the line that specifies the port to use. Each RPi/aux unit should have a different port number.
   * Add the line `PasswordAuthentication yes`
   * Add the line `PermitRootLogin yes`
6. Install dependencies (add any needed software packages to the list below): copy and paste the command(s) below:
   ```sh
   sudo apt install python2.7 git build-essentials libdbus-1-dev libdbus-glib-1-dev uuid-dev 
   ```
7. Run setup script in smart-dashcam root directory on master branch
   ```sh
   ./setup.py --aux_unit
   ```
8. Setup TCP Interface service and Wi-Fi 
   1. Checkout branch dev/interfaces from repository and go into the *Interfaces* folder
   2. Setup, build, and setup pi for DBUS and socket libraries, and Wi-Fi with the following command:
      ```sh
      make setup build pi0_setup 
      ```
9. Reboot system


# Raspbian Setup Instructions for RPi 3
1. Install dependencies (add any needed software packages to the list below): copy and paste the commands below:
   ```sh
   sudo apt install python2.7 git build-essentials libdbus-1-dev libdbus-glib-1-dev uuid-dev
   pip install PySimpleGUI27
   pip install typing
   ```
1. Run setup script in smart-dashcam root directory on master branch
   ```sh
   ./setup.py --main_unit
   ```
1. Setup TCP Interface service and Wi-Fi 
   1. Disable IPv6 on the hosted Wi-Fi network interface with the following command:
      ```sh
      sudo bash -c 'echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf'
      sysctl -p
      ```
   1. Checkout branch dev/interfaces from repository
   1. Setup, build, and setup pi for DBUS and socket libraries, and Wi-Fi with the following command from within the Interfaces directory on the dev/interfaces branch:
      ```sh
      make setup build pi3_setup 
      ```
1. Reboot system


# Alpine Linux Setup Instructions for RPi Zero W
1. Disable IPV6
    ```sh
    echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf
    sysctl -p
    ```
    Warning, you must run the script in its entirety.
    Need to test to see if disabling ipv6 is necessary anymore. If so, should disable it on all interfaces.
2. Install packages: glib, git, cmake, and gcc for compiling code and others that haven't been added.
