# Author:		
# Copyright: 	
# License:		
# Date Created:	Sept. 08, 2019
# Resources:	http://www.gnu.org/software/make/manual/make.html 
#				http://nuclear.mutantstargoat.com/articles/make/#practical-makefile
# Desc:			

## Parameters ##
CC=gcc
#CXX=g++

# Device ID file
ID_FILE=device-id

# Define names of executable files
EXE_SERV=exe_comm_tcp_server
EXE_CLIENT=exe_comm_tcp_client

# Define location of source code files
C_SERV_SRC=$(wildcard server/*.c)
C_CLIENT_SRC=$(wildcard clients/*.c)

# Define any directories containing header files other than /usr/include
C_INC=-I$(shell pwd)
# C_CLIENT_INC=-Iclients/private_includes/
# C_SERV_INC=$server/private_includes/

# Set compiler flags 
#src: https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# Compiler_FLAGS:
#	-O3:			Optimization more for code size and execution time
#	-Wall			Enables additional compiler warnings
#	-Wextra			Further enables addionational compiler warnings not included by -Wall
#	-std=gnu99		Sets what language standard is used. gnu99: GNU dialect of ISO C99
#	-ggdb			Enables debugging symbols for GDB
#	-luuid			Use the uuid library. Note that this will need to be installed on the system.
CFLAGS= -O3 -Wall -Wextra -std=gnu99 -ggdb -luuid

# Define any parameters to be passed during compile time
CDEFINES= -DUUID_FILE_NAME=\"$(ID_FILE)\"


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

.PHONY: build server client runServer runClient debugServer debugClient setup download dependencies install clean clean-all

# make build will compile all source code. This is the default if 'make' is run.
build: server client
	echo "\nFinished!"

server: clean-all
	# Compile server C code:
	$(CC) $(C_SERV_SRC) $(C_INC) $(CFLAGS) -o $(EXE_SERV)

	chmod 770 $(EXE_SERV)

client: clean-all
	# Compile client C code:
	$(CC) $(C_CLIENT_SRC) $(C_INC) $(CFLAGS) $(CDEFINES) -o $(EXE_CLIENT)
	chmod 770 $(EXE_CLIENT)

runServer: $(EXE_SERV)
	echo "\n -----------------Executing Server-----------------\n\n"
	./$(EXE_SERV)

runClient: $(EXE_CLIENT)
	echo "\n-----------------Executing Client-----------------\n\n"
	./$(EXE_CLIENT)

debugClient: $(EXE_CLIENT)
	gdb $(EXE_CLIENT)

debugServer: $(EXE_SERVER)
	gdb $(EXE_SERVER)


# make setup will go through the setup process needed to compile/run code
setup: clean-all dependencies download install clean
	echo "\nSystem setup\n"
# make dependencies will install any software/package dependencies needed (ie installs gcc on system)
dependencies:
	#For alpine linux 
		#util-linux-dev 	#for uuid generator
		#glibc 				# Compiler stuff
		#gcc				# Compiler
		#cmake				# For makefile(s)
		#git				# For git
		#build-essentials	
	#For Raspbian
		#uuid-dev			# For uuid generator
		#build-essentials	
		#git				# For git
# make download will download any items (not software) needed (ie gcc source code for a cross compiler)
download:
# make install will install any software/items needed to build/run code but that is not a software package (ie used to build a cross compiler)
install:
# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean :
# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all:
	rm -rf $(EXE_SERV) $(EXE_CLIENT) $(ID_FILE)