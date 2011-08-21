sbin_PROGRAMS = arc-infoindex-relay \
                arc-infoindex-server

pkglib_LTLIBRARIES = arc-infoindex-slapd-wrapper.la

arc_infoindex_relay_SOURCES = Relay.c

arc_infoindex_server_SOURCES = main.cpp \
                               Entry.cpp Index.cpp Policy.cpp Server.cpp \
                               Entry.h   Index.h   Policy.h   Server.h
arc_infoindex_server_LDADD = -lpthread

arc_infoindex_slapd_wrapper_la_SOURCES = SlapdWrapper.cpp
arc_infoindex_slapd_wrapper_la_LDFLAGS = -no-undefined -avoid-version -module
arc_infoindex_slapd_wrapper_la_LIBADD = -ldl

man8_MANS = arc-infoindex-relay.8 arc-infoindex-server.8
