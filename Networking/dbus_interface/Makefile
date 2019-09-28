
# Note that the directoy is /etc/dbus-1-/system.d/* for system serivice dbus entries
#     and is /etc/dbus-1/session.d/* for user session dbus
# https://medium.com/@StueyGK/static-libraries-vs-dynamic-libraries-af78f0b5f1e4
# https://randu.org/tutorials/c/libraries.php
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html

# This makefile will create shared libraries and add global header files for 
#	the dbus code folders in this directory.
# Libraries created are as follows:
#			NAME					GCC INCLUDE
#	libdashcam_dbus_server		-ldashcam_dbus_server
#	libdashcam_dbus_tcp			-ldashcam_dbus_tcp
#	libdashcam_dbus_gps			-ldashcam_dbus_gps
#	libdashcam_dbus_accl		-ldashcam_dbus_accl

lconf_file= com.dashcam.conf
conf_file= /etc/dbus-1/system.d/$(lconf_file)

# Directories
DBUS_SERVER_DIR=server
SYSTEM_GLOBAL_INC_DIR=/usr/include

CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)

CFLAGS = -g -Wall -pthread -O3 -fPIC #-Werror

all: dbus-tcp-client-lib libdashcam_dbus_server

config: $(lconf_file)
	sudo cp $< /etc/dbus-1/system.d/$<

$(conf_file): $(lconf_file)
	sudo cp -f $(lconf_file) $(conf_file)

# Create object file for the DBUS server to be used in compiling other programs as DBUS server is a library
libdashcam_dbus_server: $(conf_file)
	# Generate object files
	gcc -c $(DBUS_SERVER_DIR)/*.c -o server/libdashcam_dbus_server.o $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB) 
	
	# Generate shared (dynamic) library files(.so)
	gcc $(DBUS_SERVER_DIR)/*.o -shared -Wl,-soname,libdashcam_dbus_server.so.1 -o $(DBUS_SERVER_DIR)/libdashcam_dbus_server.so.1.0.1 -lc $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB)
	
	# ldconfig stuff
	sudo ldconfig -vn $(DBUS_SERVER_DIR)/
	
	# Change permissions and copy to system library folder
	sudo chmod 0755 $(DBUS_SERVER_DIR)/*.so.1
	sudo cp -f $(DBUS_SERVER_DIR)/*.so.1 /usr/lib/
	
	# Create link. This is what is used to find the library .so
	sudo ln -sf /usr/lib/libdashcam_dbus_server.so.1 /usr/lib/libdashcam_dbus_server.so

	# Create global includes folder for this library
	sudo mkdir -p $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server
	sudo chmod 555 $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server
	
	# Copy header files to system includes directory
	sudo cp pub_dbus.h $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server/
	sudo cp server/pub*.h $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server/
	sudo chmod 444 $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server/*.h

	echo "\n\nResults\n"
	# Test library config (Warnings about "cannot find entry symbol _start; not setting start address" are ok)
	sudo ld -ldashcam_dbus_server

	# List library header files
	ls $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_server/

dbus-tcp-client-lib: client_tcp/dbus_client.c
	gcc -c $< $(CFLAGS) $(CFLAGS_GIO) $(CFLAGS_DBUS) -o tcp_dbus_client.o

clean:
	rm -f $(DBUS_SERVER_DIR)/*dashcam*
	sudo rm -f $(conf_file)
	sudo rm -f /usr/lib/*dashcam*
	sudo ldconfig -p /usr/lib/
	sudo rm -rf /usr/include/*dashcam*


.PHONY: all clean


#Dependancies
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev