#!make

# Author:		
# Copyright: 	
# License:		
# Date Created:	Oct. 21, 2019
# Resources:
# Desc:	This makefile controls the Wi-Fi hosting scripts


# include .makerc

# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    

# Directories
SUB_DIRS= $(shell ls -d */) #List of all library sub-directories, can use $(shell ls -d */) to list all dirs



#target … : prerequisites …
#        recipe

# Where
#		target -> name of file generated
#		prerequisites ->  a file that is used as input to create the target
#		recipe -> actions taken by make. Tab is required.

.PHONY: pi3_setup pi0_setup build setup download dependencies install clean clean-all

build: $(SUB_DIRS)
	# Make scripts executable
	sudo chmod +x *.sh

    @echo "Built all services\n\n"

pi3_setup:
	sudo ./PI_HOST_WIFI_NETWORK_INSTALL.sh
	sudo ./PI_HOST_WIFI_NETWORK_ENABLE.sh
	@echo "\n\n\n\nTHE SYSTEM NEEDS TO REBOOT\n\n\n\n"
    @echo "Setup system for Raspberry Pi 3!\n\n"

pi0_setup:
	sudo mv /etc/wpa_supplicant/wpa_supplicant.conf backups/
	sudo cp wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf
    @echo "Setup system for Raspberry Pi 0!\n\n"

# make setup will go through the setup process needed to compile/run code
setup: clean-all dependencies download install clean
    # # Run make setup for all sub-directories
    # for dir in $(SUB_DIRS);                 \
    #     do                                  \
    #       $(MAKE) --directory=$$dir setup;  \
    # done
    # echo "\nSystem setup\n"

# make dependencies will install any software/package dependencies needed (ie installs gcc on system)
dependencies:
    # # Run make dependencies for all sub-directories
    # for dir in $(SUB_DIRS);                         \
    #     do                                          \
    #       $(MAKE) --directory=$$dir dependencies;   \
    # done

# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:
    # # Run make download for all sub-directories
    # for dir in $(SUB_DIRS);                     \
    #     do                                      \
    #       $(MAKE) --directory=$$dir download;   \
    # done

# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:
    # # Run make install for all sub-directories
    # for dir in $(SUB_DIRS);                     \
    #     do                                      \
    #       $(MAKE) --directory=$$dir install;    \
    # done

# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean :
    # # Run make clean for all sub-directories
    for dir in $(SUB_DIRS);                 \
        do                                  \
          $(MAKE) --directory=$$dir clean;  \
    done

# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all:
    # Run make clean-all for all sub-directories
    for dir in $(SUB_DIRS);                     \
        do                                      \
          $(MAKE) --directory=$$dir clean-all;  \
    done
 
