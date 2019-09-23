
# Note that the directoy is /etc/dbus-1-/system.d/* for system serivice dbus entries
#     and is /etc/dbus-1/session.d/* for user session dbus
lconf_file= com.dashcam.conf
conf_file= /etc/dbus-1/system.d/$(lconf_file)

CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)

CFLAGS = -g -Wall -pthread -O3 #-Werror
CLIBS = `pkg-config --libs dbus-1`

all: dbus-server dbus-client

# Note, run this first!!!
config: com.dashcam.conf
	sudo cp $< /etc/dbus-1/system.d/$<

$(conf_file): $(lconf_file)
	sudo cp -f $(lconf_file) $(conf_file)

dbus-server: dbus_server.c $(conf_file)
	gcc $< $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB) -o exe_dbus_server

dbus-client: dbus_client.c
	gcc $< $(CFLAGS) $(CFLAGS_GIO) $(CFLAGS_DBUS) -o exe_dbus_client

clean:
	rm -f exe_dbus_server
	rm -f exe_dbus_client
	sudo rm -f $(conf_file)


.PHONY: all clean


#Dependancies
# 	Alpine Linux:
#		dbus-dev
#		dbus-glib-dev
#		?more?
#	Raspbian:
#		libdbus-1-dev
#		libdbus-glib-1-dev