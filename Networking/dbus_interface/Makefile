CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)

CFLAGS = -g -Wall -pthread -O3 #-Werror
CLIBS = `pkg-config --libs dbus-1`

all: dbus-server dbus-client

dbus-server: dbus_server.c
	gcc $< $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB) -o exe_dbus_server

dbus-client: dbus_client.c
	gcc $< $(CFLAGS) $(CFLAGS_GIO) $(CFLAGS_DBUS) -o exe_dbus_client

clean:
	rm -f exe_dbus_server
	rm -f exe_dbus_client


.PHONY: all clean


#Dependancies
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev