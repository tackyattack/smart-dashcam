
]# Samba Bash Files
- sambapiZeroBash_restoreConfig.sh is a script that replaces the original samba config file and removes the backups folder
- sambapiZeroBash.sh is a script that installs samba-client, adds a backup of the original samba config file, and brings in the updated configuration file for samba.
- sambapi3_AUX_MOUNT_Bash.sh is a script that takes TWO Parameters, the FIRST being the IP Address of the auxillary pi, and the SECOND being the name of the directory that you would like it to be called. This also mounts the drive to a folder in /Recordings/[Second Parameter]
- sambapi3_AUX_UNMOUNT_Bash.sh is a script that takes ONE Parameter, being the name of the directory that you want to unmount from.  This directory is in /Recordings/[FIRST Parameter]
- samba_config_for_pi3.conf is the config file used for the pi3 for all the samba mounting purposes
- samba_config_for_piZero.conf is the config file used for the piZero or "Auxillary Pi's" for the samba nmounting purposes
