#!make

# Author:		
# Copyright: 	
# License:		
# Date Created:	Oct. 6, 2019
# Resources: Note that the directoy is /etc/dbus-1-/system.d/* for system serivice dbus entries
#            and is /etc/dbus-1/session.d/* for user session dbus
#            https://medium.com/@StueyGK/static-libraries-vs-dynamic-libraries-af78f0b5f1e4
#            https://randu.org/tutorials/c/libraries.php
#            http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
#            https://www.oreilly.com/library/view/managing-projects-with/0596006101/ch06.html
# Desc:	
#       This makefile will create shared libraries and add global header files for 
#   	the dbus code folders in this directory.
#       
#       Libraries created are as follows:
#   			NAME					GCC INCLUDE                         GCC #include < FILE >
#   	libdashcam_tcp_dbus_srv         -ldashcam_tcp_dbus_srv              dashcam_dbus/pub_dbus.h && dashcam_dbus/pub_tcp_dbus_srv.h
#   	libdashcam_tcp_dbus_clnt        -ldashcam_tcp_dbus_clnt             dashcam_dbus/pub_dbus.h && dashcam_dbus/pub_tcp_dbus_clnt.h
#   	libdashcam_sensors_dbus_srv     -ldashcam_sensors_dbus_srv
#   	libdashcam_sensors_dbus_clnt    -ldashcam_sensors_dbus_clnt

include .makerc

# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    

# Directories
LIB_DIRS= tcp_dbus_server/ tcp_dbus_client/ #List of all library sub-directories, can use $(shell ls -d */) to list all dirs



#target … : prerequisites …
#        recipe

# Where
#		target -> name of file generated
#		prerequisites ->  a file that is used as input to create the target
#		recipe -> actions taken by make. Tab is required.

.PHONY: build setup download dependencies install clean clean-all libdashcam_tcp_dbus_srv libdashcam_tcp_dbus_clnt

build: $(LIB_DIRS)
    # Run make build for all sub-directories
    for dir in $(LIB_DIRS);                 \
        do                                  \
          $(MAKE) --directory=$$dir build;  \
    done
        
    @echo "Built all libraries\n\n"


# Create DBUS server dynamic library to be used in other programs as a library
libdashcam_tcp_dbus_srv: 
    $(MAKE) -C tcp_dbus_server/

# Create DBUS server dynamic library to be used in other programs as a library
libdashcam_tcp_dbus_clnt: 
    $(MAKE) -C tcp_dbus_client/


# make setup will go through the setup process needed to compile/run code
setup: clean-all dependencies download install clean
    # # Run make setup for all sub-directories
    # for dir in $(LIB_DIRS);                 \
    #     do                                  \
    #       $(MAKE) --directory=$$dir setup;  \
    # done
    # echo "\nSystem setup\n"

# make dependencies will install any software/package dependencies needed (ie installs gcc on system)
dependencies:
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev

    # # Run make dependencies for all sub-directories
    # for dir in $(LIB_DIRS);                         \
    #     do                                          \
    #       $(MAKE) --directory=$$dir dependencies;   \
    # done

# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:
    # # Run make download for all sub-directories
    # for dir in $(LIB_DIRS);                     \
    #     do                                      \
    #       $(MAKE) --directory=$$dir download;   \
    # done

# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:
    # # Run make install for all sub-directories
    # for dir in $(LIB_DIRS);                     \
    #     do                                      \
    #       $(MAKE) --directory=$$dir install;    \
    # done

# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean :
    # # Run make clean for all sub-directories
    for dir in $(LIB_DIRS);                 \
        do                                  \
          $(MAKE) --directory=$$dir clean;  \
    done

# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all:
    # Run make clean-all for all sub-directories
    for dir in $(LIB_DIRS);                     \
        do                                      \
          $(MAKE) --directory=$$dir clean-all;  \
    done

    sudo rm -f $(SYSTEM_GLOBAL_LIB_DIR)/*dashcam*
    sudo rm -rf $(SYSTEM_GLOBAL_INC_DIR)/*dashcam*
    sudo rm -rf $(SYS_DBUS_SRV_CONF_DIR)/*dashcam*
    sudo ldconfig -p $(SYSTEM_GLOBAL_LIB_DIR)/
