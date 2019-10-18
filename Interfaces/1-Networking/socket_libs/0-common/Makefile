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
#       This makefile will create static libraries for the common socket code in this directory.
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

# Define library names
     #Long name of LIB_INC_NAME (libcommon -> -lcommon)
LIB_NAME=libcommon
    # This flag will need to be included to use this static library
LIB_INC_NAME=$(LIB_COMMON)

# Directories
    # This directory will need the -L$(LIB_DIR) flag to use this static library
LIB_DIR=$(COMMON_LIB_DIR)
OBJS=objs/

# Compiler Flags
CFLAGS=$(GLOBAL_CFLAGS)

# Compilers
CC=gcc
CXX=g++

# Define location of source code files
C_SRC=$(wildcard *.c)

# Define public header to be copied to system includes directory
PUB_HEADER=$(wildcard pub*.h)

# Flags for static library code
# src: https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
#	   https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/GCC/create-libraries/index
#	-fPIC			Generate position-independent code (PIC) suitable for use in a shared library
STATIC_LIB_CFLAGS = -fPIC

# Define any directories containing header files other than /usr/include
# C_INC=-Ipath

# Define library paths and libraries in addition to /usr/lib
#   Use -Lpath and -lname
# LDFLAGS=-Lpath/

.PHONY: build setup download dependencies install clean clean-all $(LIB_NAME)

build: $(LIB_NAME) 
    @echo "-----------------Done Building Static Library $(LIB_NAME)-----------------\n"

# Create DBUS client dynamic library to be used in other programs as a library
$(LIB_NAME): 
    @# Create folders
    mkdir -p $(OBJS)
    mkdir -p $(LIB_DIR)

    @# Generate object files
    $(CC) -c $(C_SRC) -o $(OBJS)/$(LIB_NAME).o $(CFLAGS) $(STATIC_LIB_CFLAGS)

    @# Generate static library file(*.a)
    ar rcs $(LIB_DIR)/$(LIB_NAME).a $(OBJS)/*.o

    @# Copy header files to system includes directory
    cp $(PUB_HEADER) $(LIB_DIR)
    chmod 444 $(LIB_DIR)/*.h

    @# This will test if successful (You have to read the output)
    @echo "\n\n-----------------Results for $(LIB_NAME)-----------------\n"
    @echo "Static Lib Name: $(LIB_NAME)"
    @echo "Static Lib Include: $(LIB_INC_NAME)"
    @echo "Static Dir Include: -L$(LIB_DIR)"

    @echo "\n"
    # List library header files (ignore *.a files)
    @ls $(LIB_DIR)
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
    rm -rf $(LIB_DIR)
