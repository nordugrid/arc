libexec_PROGRAMS = smtp-send
libexec_SCRIPTS  = smtp-send.sh
noinst_LTLIBRARIES = libmail.la

smtp_send_SOURCES = smtp-send.c
smtp_send_LDADD = $(LIBRESOLV)

libmail_ladir=$(includedir)
libmail_la_HEADERS=send_mail.h
libmail_la_SOURCES=send_mail.cpp 
libmail_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
#libmail_la_LIBADD=../files/libfiles.la ../run/librun.la ../conf/libconf.la
