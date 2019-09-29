
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
SYSTEM_GLOBAL_INC_DIR=/usr/include
SYSTEM_GLOBAL_LIB_DIR=/usr/lib

# Directories
DBUS_SERVER_DIR=server
DBUS_CLIENT_TCP_DIR=client_tcp


CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)

CFLAGS = -g -Wall -pthread -O3 -fPIC -Werror -Wl,--start-group

all: libdashcam_dbus_server libdashcam_dbus_tcp

config: $(lconf_file)
	sudo cp $< /etc/dbus-1/system.d/$<

$(conf_file): $(lconf_file)
	sudo cp -f $(lconf_file) $(conf_file)

# Create DBUS server dynamic library to be used in other programs as a library
libdashcam_dbus_server: $(conf_file)
	# Generate object files
	gcc -c $(DBUS_SERVER_DIR)/*.c -o $(DBUS_SERVER_DIR)/libdashcam_dbus_server.o $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB) 
	
	# Generate shared (dynamic) library files(.so)
	gcc $(DBUS_SERVER_DIR)/*.o -shared -Wl,-soname,libdashcam_dbus_server.so.1 -o $(DBUS_SERVER_DIR)/libdashcam_dbus_server.so.1.0.1 -lc $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB)
	
	# ldconfig stuff
	sudo ldconfig -vn $(DBUS_SERVER_DIR)/
	
	# Change permissions and copy to system library folder
	sudo chmod 0755 $(DBUS_SERVER_DIR)/*.so.1
	sudo cp -f $(DBUS_SERVER_DIR)/*.so.1 $(SYSTEM_GLOBAL_LIB_DIR)/
	
	# Create link. This is what is used to find the library .so
	sudo ln -sf $(SYSTEM_GLOBAL_LIB_DIR)/libdashcam_dbus_server.so.1 $(SYSTEM_GLOBAL_LIB_DIR)/libdashcam_dbus_server.so

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

# Create DBUS server dynamic library to be used in other programs as a library
libdashcam_dbus_tcp: 
	# A note for GCC, orders of the parameters matter. The wrong order can cause no errors/warnings but the resulting compiled file
	#	to be incorrect/broken. Look up gcc parameter order. Supposedly, adding the -Wl,--start-group flag can fix this
	# Generate object files
	gcc $(CFLAGS) -c $(DBUS_CLIENT_TCP_DIR)/*.c -o $(DBUS_CLIENT_TCP_DIR)/libdashcam_dbus_tcp.o $(CFLAGS_GIO) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB)
	
	# Generate shared (dynamic) library files(.so)
	gcc $(DBUS_CLIENT_TCP_DIR)/*.o -shared -Wl,-soname,libdashcam_dbus_tcp.so.1 -o $(DBUS_CLIENT_TCP_DIR)/libdashcam_dbus_tcp.so.1.0.1 -lc $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB) $(CFLAGS_GIO)
	
	# ldconfig stuff
	sudo ldconfig -vn $(DBUS_CLIENT_TCP_DIR)/
	
	# Change permissions and copy to system library folder
	sudo chmod 0755 $(DBUS_CLIENT_TCP_DIR)/*.so.1
	sudo cp -f $(DBUS_CLIENT_TCP_DIR)/*.so.1 $(SYSTEM_GLOBAL_LIB_DIR)/
	
	# Create link. This is what is used to find the library .so
	sudo ln -sf $(SYSTEM_GLOBAL_LIB_DIR)/libdashcam_dbus_tcp.so.1 $(SYSTEM_GLOBAL_LIB_DIR)/libdashcam_dbus_tcp.so

	# Create global includes folder for this library
	sudo mkdir -p $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp
	sudo chmod 555 $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp
	
	# Copy header files to system includes directory
	sudo cp pub_dbus.h $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp/
	sudo cp $(DBUS_CLIENT_TCP_DIR)/pub*.h $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp/
	sudo chmod 444 $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp/*.h

	echo "\n\nResults\n"
	# Test library config (Warnings about "cannot find entry symbol _start; not setting start address" are ok)
	sudo ld -ldashcam_dbus_tcp

	# List library header files
	ls $(SYSTEM_GLOBAL_INC_DIR)/dashcam_dbus_tcp/

# dbus-tcp-client-lib: client_tcp/dbus_client.c
# 	gcc -c $< $(CFLAGS) $(CFLAGS_GIO) $(CFLAGS_DBUS) -o tcp_dbus_client.o

clean:
	rm -f $(DBUS_SERVER_DIR)/*dashcam*
	rm -f $(DBUS_CLIENT_TCP_DIR)/*dashcam*
	sudo rm -f $(conf_file)
	sudo rm -f $(SYSTEM_GLOBAL_LIB_DIR)/*dashcam*
	sudo rm -rf /usr/include/*dashcam*
	sudo ldconfig -p $(SYSTEM_GLOBAL_LIB_DIR)/


.PHONY: all clean


#Dependancies
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev