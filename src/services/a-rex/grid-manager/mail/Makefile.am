libexec_PROGRAMS = smtp-send
libexec_SCRIPTS  = smtp-send.sh
noinst_LTLIBRARIES = libmail.la

smtp_send_SOURCES = smtp-send.c
smtp_send_LDADD   = $(LIBRESOLV)

libmail_la_SOURCES  = send_mail.cpp send_mail.h
libmail_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
#libmail_la_LIBADD=../files/libfiles.la ../run/librun.la ../conf/libconf.la
