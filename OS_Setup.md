
# Notes
  - Ethernet devices that are Davicom device (more specifically that use the DM9601 or DM96xx drivers) do not work on the Raspberry Pi's
  - If a network device is attached for which Linux does not have a driver, it seems that this screws with Linux such that it has random networking problems such as not being able to change Wi-Fi networks.
  - It seems easiest to change Wi-Fi networks by altering the /etc/wpa_supplicant/wpa_supplicant.conf file such that the network you want to connect to is listed first followed by rebooting the system
  - You can't use a hostname as the server address in the socket client library due to RPi OS bug that is out of my control or ability to fix.

# Raspbian Setup Instructions for RPi Zero W

1. Burn the [Raspbian Buster Lite](https://www.raspberrypi.org/downloads/raspbian/) image to a microSD card
2. Boot RPi and log in with default user: *pi* and password: *raspberry*
3. Change the user password to *dashcam2019* via command 
      ```sh
      passwd pi
      ```
4. Using the command below, do the following:
      ```sh 
      sudo raspi-config
      ```
   * [development only]  Enable SSH
   * [optional] Set hostname to *dashcam_aux0* where the '0' is replaced with a device number that should be unique but in reality this doesn't matter as hostnames are broken on Raspbian.

5. [development only] Type 
      ```sh 
      sudo nano /etc/ssh/sshd_config
      ```
   * Modify the line that specifies the port to use. Each RPi/aux unit should have a different port number.
   * Add the line `PasswordAuthentication yes`
   * Add the line `PermitRootLogin yes`
   * Add the line `TCPKeepAlive yes`
   * Add the line `ClientAliveInterval 30`
   * Add the line `ClientAliveCountMax 99999`

6. Install dependencies (add any needed software packages to the list below): copy and paste the command(s) below:
      ```sh
      sudo apt update
      sudo apt install python2.7 git build-essentials
      ```
7. Clone smart-dashcam into `/home/pi`
      ```sh
      cd ~ && git clone https://git-classes.mst.edu/hhbkv6/smart-dashcam.git
      ```
8.  Disable IPv6 on the wireless interface:
      ```sh
      sudo bash -c 'echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf'
      sudo sysctl -p
      ```
9. Run setup script in smart-dashcam root directory on master branch.
      ```sh
      cd ~/smart-dashcam && ./setup.py --aux_unit
      ```
10. Reboot system
      ```sh
      sudo reboot
      ```


# Raspbian Setup Instructions for RPi 3
1. Burn the [Raspbian Buster with desktop](https://www.raspberrypi.org/downloads/raspbian/) image to a microSD card
1. Boot RPi and log in with default user: *pi* and password: *raspberry*
1. Change the user password to *dashcam2019* via command 
      ```sh
      passwd pi
      ```
1. Using the command below, do the following:
      ```sh 
      sudo raspi-config
      ```
   * [development only]  Enable SSH
   * [optional] Set hostname to *dashcam_aux0* where the '0' is replaced with a device number that should be unique but in reality this doesn't matter as hostnames are broken on Raspbian.

1. [development only] Type 
      ```sh 
      sudo nano /etc/ssh/sshd_config
      ```
   * Modify the line that specifies the port to use. Each RPi/aux unit should have a different port number.
   * Add the line `PasswordAuthentication yes`
   * Add the line `PermitRootLogin yes`
   * Add the line `TCPKeepAlive yes`
   * Add the line `ClientAliveInterval 30`
   * Add the line `ClientAliveCountMax 99999`

1. Install dependencies (add any needed software packages to the list below): copy and paste the commands below:
      ```sh
      sudo apt update
      sudo apt install python2.7 git build-essentials
      ```
1. Disable IPv6 on the hosted Wi-Fi network interface with the following command:
      ```sh
      sudo bash -c 'echo "net.ipv6.conf.wlan0.disable_ipv6 = 1" >> /etc/sysctl.conf'
      sudo sysctl -p
      ```
1. Clone smart-dashcam into `/home/pi`
      ```sh
      cd ~ && git clone https://git-classes.mst.edu/hhbkv6/smart-dashcam.git
      ```
1. Run setup script in smart-dashcam root directory on master branch
      ```sh
      cd ~/smart-dashcam && ./setup.py --main_unit
      ```
1. In /boot/config.txt, increase GPU memory
      ```
      gpu_mem=256
      ```
1. Setup LCD by following Adafruit's [tutorial](https://learn.adafruit.com/adafruit-pitft-28-inch-resistive-touchscreen-display-raspberry-pi/easy-install-2) and script.
   1. Ensure that you choose HDMI mirror in the script to allow for OpenGL display.
   1. In /boot/config.txt, ensure that the dtoverlay is set properly. Your display may have a different
   driver so the display type may be different. Look at the [goodtft scripts](https://github.com/goodtft/LCD-show) for
   examples. Note: speed should be as high as possible without distorting colors. Mine was 24MHz.
      ```
      dtoverlay=tft35a:rotate=90,speed=24000000,fps=60
      ```
1. Reboot system
      ```sh
      sudo reboot
      ```
