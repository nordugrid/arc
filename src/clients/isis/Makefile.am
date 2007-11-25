noinst_PROGRAMS = iic

iic_SOURCES  = main.cpp iic.cpp iic.h
iic_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
iic_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
iic_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la
