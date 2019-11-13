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
# Desc:	
#       This makefile will create shared libraries and add global header files for 
#   	the dbus code in this directory.
#       
#       Libraries created are as follows:
#   			NAME					GCC INCLUDE                         GCC #include < FILE >
#   	libdashcam_tcp_dbus_srv         -ldashcam_tcp_dbus_srv              dashcam_dbus/pub_dbus.h && dashcam_dbus/pub_tcp_dbus_srv.h

-include ../.makerc # This file contains the definitions of many of the variables used including the following
                    # SYS_DBUS_SRV_CONF_DIR
                    # SYSTEM_GLOBAL_INC_DIR
                    # SYSTEM_GLOBAL_LIB_DIR
                    # CFLAGS_DBUS
                    # CFLAGS_DBUS_GLIB
                    # CFLAGS_GIO
                    # GLOBAL_CFLAGS

# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    


## Parameters ##

# DBUS Server Conf file
lconf_file= com.dashcam.tcp.conf
SYS_DBUS_CONF_FILE=$(SYS_DBUS_SRV_CONF_DIR)/$(lconf_file)

# Names
LIB_NAME=libdashcam_tcp_dbus_srv
LIB_INC_NAME=ldashcam_tcp_dbus_srv

# Directories
SYS_INC_DIR=$(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus/
OBJS=objs/

# Compiler Flags
CFLAGS=$(GLOBAL_CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB)

# Compilers
CC=gcc
CXX=g++

# Define location of source code files
C_SRC=$(wildcard *.c)

# Define public header to be copied to system includes directory
PUB_HEADER=$(wildcard pub*.h)

# Define any directories containing header files other than /usr/include
# C_INC=-I$(shell pwd)


# Define any parameters to be passed during compile time
# CDEFINES=


# Define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath:
# LDFLAGS=





#target … : prerequisites …
#        recipe

# Where
#		target -> name of file generated
#		prerequisites ->  a file that is used as input to create the target
#		recipe -> actions taken by make. Tab is required.


.PHONY: build setup download dependencies install clean clean-all $(LIB_NAME)

build: $(LIB_NAME) 
    @echo "Built Library $(LIB_NAME)\n"

$(SYS_DBUS_CONF_FILE): $(lconf_file)
    sudo cp -f $< $(SYS_DBUS_CONF_FILE) # Equivalent to line below
    @# sudo cp -f $(lconf_file) $(SYS_DBUS_CONF_FILE)

# Create DBUS server dynamic library to be used in other programs as a library
$(LIB_NAME): $(SYS_DBUS_CONF_FILE)
    @# Create objs folder
    mkdir -p $(OBJS)

    @# Generate object files
    $(CC) -c $(C_SRC) -o $(OBJS)/$(LIB_NAME).o $(CFLAGS)
    
    @# Generate shared (dynamic) library files(.so)
    $(CC) $(OBJS)/*.o -shared -Wl,-soname,$(LIB_NAME).so.1 -o $(OBJS)/$(LIB_NAME).so.1.0.1 -lc $(CFLAGS)
    
    @# ldconfig stuff
    sudo ldconfig -vn $(OBJS)/
    
    @# Change permissions and copy to system library folder
    sudo chmod 0755 $(OBJS)/*.so.1
    sudo cp -f $(OBJS)/*.so.1 $(SYSTEM_GLOBAL_LIB_DIR)/
    
    @# Create link. This is what is used to find the library .so
    sudo ln -sf $(SYSTEM_GLOBAL_LIB_DIR)/$(LIB_NAME).so.1 $(SYSTEM_GLOBAL_LIB_DIR)/$(LIB_NAME).so

    @# Create global includes folder for this library
    sudo mkdir -p $(SYS_INC_DIR)
    sudo chmod 555 $(SYS_INC_DIR)
    
    @# Copy header files to system includes directory
    sudo cp ../pub_dbus.h $(SYS_INC_DIR)
    sudo cp $(PUB_HEADER) $(SYS_INC_DIR)
    sudo chmod 444 $(SYS_INC_DIR)/*.h

    @# This will test if successful (You have to read the output)
    @echo "\n\nResults\n"
    # Test library config (Warnings about "cannot find entry symbol _start; not setting start address" are ok)
    @ld -o $(OBJS)/temp -$(LIB_INC_NAME)
    @rm $(OBJS)/temp

    @echo "\n"
    # List library header files
    @ls $(SYS_INC_DIR)
    @echo "\n"


# make setup will go through the setup process needed to compile/run code
setup: clean-all dependencies download install clean
    echo "\nSetup!\n"

# make dependencies will install any software/package dependencies needed (ie installs gcc on system)
dependencies:
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev

# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:

# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:

# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean:
    rm -rf $(OBJS)

# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all: clean
    sudo rm -f $(SYS_DBUS_CONF_FILE)
    sudo rm -f $(SYSTEM_GLOBAL_LIB_DIR)/$(LIB_NAME)*
    sudo rm -rf $(SYS_INC_DIR)
    sudo ldconfig -p $(SYSTEM_GLOBAL_LIB_DIR)
