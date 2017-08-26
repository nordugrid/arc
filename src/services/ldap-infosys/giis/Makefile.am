sbin_PROGRAMS = arc-infoindex-relay \
                arc-infoindex-server

arc_infoindex_relay_SOURCES = Relay.c

arc_infoindex_server_SOURCES = main.cpp \
                               Entry.cpp Index.cpp Policy.cpp Server.cpp \
                               Entry.h   Index.h   Policy.h   Server.h
arc_infoindex_server_LDADD = -lpthread

man8_MANS = arc-infoindex-relay.8 arc-infoindex-server.8
