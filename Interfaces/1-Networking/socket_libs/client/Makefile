#!make

# Author:		
# Copyright: 	
# License:		
# Date Created:	Oct. 6, 2019
# Resources:
#            https://medium.com/@StueyGK/static-libraries-vs-dynamic-libraries-af78f0b5f1e4
#            https://randu.org/tutorials/c/libraries.php
#            http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
# Desc:	
#       This makefile will create shared libraries and add global header files for 
#   	the socket code in this directory.
#       
#       Libraries created are as follows:
#       	NAME        					GCC INCLUDE                     GCC #include < FILE >
#   	libdashcam_socket_clnt          -ldashcam_socket_clnt               dashcam_sockets/pub_socket_client.h

-include ../.makerc # This file contains the definitions of many of the variables used including the following
                    # SYSTEM_GLOBAL_INC_DIR
                    # SYSTEM_GLOBAL_LIB_DIR
                    # GLOBAL_CFLAGS
					# COMMON_LIB_DIR
					# LIB_COMMON

# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    


## Parameters ##

# Device ID file
ID_FILE=device-id

# Define any parameters to be passed during compile time
CDEFINES= -DUUID_FILE_NAME=\"$(ID_FILE)\"

# Names
LIB_NAME=libdashcam_socket_clnt
LIB_INC_NAME=ldashcam_socket_clnt

# Directories
SYS_INC_DIR=$(SYSTEM_GLOBAL_INC_DIR)/dashcam_sockets/
OBJS=objs/

# Compiler Flags
CFLAGS=$(GLOBAL_CFLAGS) -pthread -luuid

# Compilers
CC=gcc
CXX=g++

# Define location of source code files
C_SRC=$(wildcard *.c)

# Define public header to be copied to system includes directory
PUB_HEADER=$(wildcard pub*.h)

# Define any directories containing header files other than /usr/include
C_INC=-I$(COMMON_LIB_DIR)

# Define library paths and libraries in addition to /usr/lib
#   Use -Lpath and -llib
LDFLAGS=-L$(COMMON_LIB_DIR) $(LIB_COMMON)

.PHONY: build setup download dependencies install clean clean-all $(LIB_NAME)

build: $(COMMON_LIB_DIR) $(LIB_NAME) 
    @echo "-----------------Done Building Library $(LIB_NAME)-----------------\n"

# Create DBUS client dynamic library to be used in other programs as a library
$(LIB_NAME): 
    @# Create objs folder
    mkdir -p $(OBJS)

    @# Generate object files
    $(CC) -c $(C_SRC) -o $(OBJS)/$(LIB_NAME).o $(CFLAGS) $(LDFLAGS) $(C_INC) $(CDEFINES)
    
    @# Generate shared (dynamic) library files(.so)
    $(CC) $(OBJS)/*.o -shared -Wl,-soname,$(LIB_NAME).so.1 -o $(OBJS)/$(LIB_NAME).so.1.0.1 -lc $(CFLAGS) $(LDFLAGS) $(C_INC) $(CDEFINES)
    
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
    sudo cp $(PUB_HEADER) $(SYS_INC_DIR)
    sudo chmod 444 $(SYS_INC_DIR)/*.h

    @# This will test if successful (You have to read the output)
    @echo "\n\n-----------------Results for $(LIB_NAME)-----------------\n"
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
    # Depends on libcommon in ../common/

# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:

# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:

# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean:
    rm -rf $(OBJS)

# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all: clean
    sudo rm -f $(SYSTEM_GLOBAL_LIB_DIR)/$(LIB_NAME)*
    sudo rm -rf $(SYS_INC_DIR)
    sudo ldconfig -p $(SYSTEM_GLOBAL_LIB_DIR)
