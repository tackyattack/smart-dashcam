# Author:		
# Copyright: 	
# License:		
# Date Created:	Sept. 27, 2019
# Resources:	http://www.gnu.org/software/make/manual/make.html 
#				http://nuclear.mutantstargoat.com/articles/make/#practical-makefile
# Desc:			

## Parameters ##
CC=gcc
CXX=g++

# Define names of executable files
EXE=../exe_test_socket_server

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
#	-O3:					Optimization more for code size and execution time
CFLAGS=-Wall -Wextra -std=gnu99 -ggdb -Werror -ldashcam_socket_srv -O3

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

.PHONY: build socket_srv run setup download dependencies install clean clean-all

# make build will compile all source code. This is the default if 'make' is run.
build: socket_srv
	echo "\nFinished!"

socket_srv: clean-all
	# Compile test socket_srv C code:
	$(CC) -o $(EXE) $(C_SRC) $(C_INC) $(CFLAGS)

	chmod 770 $(EXE)

run: $(EXE)
	echo "\n -----------------Executing TEST Socket Server-----------------\n\n"
	./$(EXE)

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
	rm -rf $(EXE)