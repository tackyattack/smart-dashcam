# Author:		
# Copyright: 	
# License:		
# Date Created:	Sept. 27, 2019
# Resources:	http://www.gnu.org/software/make/manual/make.html 
#				http://nuclear.mutantstargoat.com/articles/make/#practical-makefile
# Desc:			


# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    

## Parameters ##
CC=gcc
CXX=g++

# Define names of executable files
EXE=../exe_service_tcp_server
SERVICES_DIR=/usr/bin/
SYSTEMD_UNIT_DIR=/etc/systemd/system/

# Define location of source code files
C_SRC=$(wildcard *.c)

# Define any directories containing header files other than /usr/include
C_INC=-I$(shell pwd) #-I../../dbus_interface/

# Set compiler flags 
#src: https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# Compiler_FLAGS:
#	-Wall					Enables additional compiler warnings
#	-Wextra					Further enables addionational compiler warnings not included by -Wall
#	-std=gnu99				Sets what language standard is used. gnu99: GNU dialect of ISO C99
#	-ggdb					Enables debugging symbols for GDB
#	-Werror					Treat warnings as errors
#	-ldashcam_socket_srv	Include dynamic (shared) library containing the dashcam socket server function calls
#	-ldashcam_tcp_dbus_srv	Include dynamic (shared) library containing the dashcam dbus server (for tcp server) function calls	
#	-pthread				Pthread lib
#	-O3:					Optimization more for code size and execution time
CFLAGS=-Wall -Wextra -std=gnu99 -ggdb -Werror -ldashcam_socket_srv -ldashcam_tcp_dbus_srv -pthread -O3

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

.PHONY: pi3_setup pi0_setup build service_tcp_srv run setup download dependencies install clean clean-all

# make build will compile all source code. This is the default if 'make' is run.
build: service_tcp_srv
    @echo "\nFinished!"

service_tcp_srv: 
    # Compile test service_tcp_srv C code:
    $(CC) -o $(EXE) $(C_SRC) $(C_INC) $(CFLAGS)
    chmod 770 $(EXE)

run: $(EXE)
    @echo "\n -----------------Executing TCP Server Service -----------------\n\n"
    sudo ./$(EXE)

$(EXE): service_tcp_srv
    @echo "Done\n\n"

pi3_setup: $(EXE)
    sudo cp -f $(EXE) $(SERVICES_DIR)/dashcam_tcp_server
    sudo chmod +x $(SERVICES_DIR)/dashcam_tcp_server

    sudo cp -f dashcam_tcp_server.service $(SYSTEMD_UNIT_DIR)
    sudo chmod 0755 $(SYSTEMD_UNIT_DIR)/dashcam_tcp_server.service
    sudo systemctl enable dashcam_tcp_server
    sudo systemctl daemon-reload
    sudo systemctl restart dashcam_tcp_server
    @echo "Setup system for Raspberry Pi 3!\n\n"

pi0_setup: $(EXE)
    @echo "Setup system for Raspberry Pi 0!\n\n"

# make setup will go through the setup process needed to compile/run code
setup: clean-all dependencies download install clean
    echo "\nSystem setup\n"
# make dependencies will install any software/package dependencies needed (ie installs gcc on system)
dependencies:
    # Requires libdashcam_dbus_server, this can be built/setup by running `make libdashcam_dbus_server` in the Networking/dbus_interface folder\

# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:
# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:
# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean :
# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all:
    sudo rm -rf $(EXE) $(SYSTEMD_UNIT_DIR)/dashcam_tcp_server.service $(SERVICES_DIR)/dashcam_tcp_server
    
