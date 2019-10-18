#!make

# Author:		
# Copyright: 	
# License:		
# Date Created:	Oct. 6, 2019
# Resources: 
#            https://medium.com/@StueyGK/static-libraries-vs-dynamic-libraries-af78f0b5f1e4
#            https://randu.org/tutorials/c/libraries.php
#            http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
#            https://www.oreilly.com/library/view/managing-projects-with/0596006101/ch06.html
#       Libraries created are as follows:
#       	NAME        					GCC INCLUDE                     GCC #include < FILE >
#   	libdashcam_socket_srv           -ldashcam_socket_srv                dashcam_sockets/pub_socket_server.h
#   	libdashcam_socket_clnt          -ldashcam_socket_clnt               dashcam_sockets/pub_socket_client.h

# include .makerc

# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=    

# Directories
LIB_DIRS= $(shell ls -d */) #List of all library sub-directories, can use $(shell ls -d */) to list all dirs



#target … : prerequisites …
#        recipe

# Where
#		target -> name of file generated
#		prerequisites ->  a file that is used as input to create the target
#		recipe -> actions taken by make. Tab is required.

.PHONY: build setup download dependencies install clean clean-all common_lib libdashcam_socket_srv libdashcam_socket_clnt

build: $(LIB_DIRS)
    # Run make build for all sub-directories
    for dir in $(LIB_DIRS);                 \
        do                                  \
          $(MAKE) --directory=$$dir build;  \
    done
        
    @echo "Built all libraries\n\n"

common_lib:
    $(MAKE) -C 0-common/

# Create socket server dynamic library to be used in other programs as a library
libdashcam_socket_srv: common_lib
    $(MAKE) -C server/

# Create socket cliennt dynamic library to be used in other programs as a library
libdashcam_socket_clnt: common_lib
    $(MAKE) -C client/

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
    # # Run make dependencies for all sub-directories
    # for dir in $(LIB_DIRS);                         \
    #     do                                          \
    #       $(MAKE) --directory=$$dir dependencies;   \
    # done
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
