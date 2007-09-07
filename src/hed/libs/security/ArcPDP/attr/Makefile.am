lib_LTLIBRARIES= libarcattr.la
noinst_PROGRAMS=
SOURCES =

libarcattr_ladir=$(includedir)
libarcattr_la_HEADERS=AttributeValue.h RequestAttribute.h AttributeProxy.h AttributeFactory.h ArcAttributeFactory.h StringAttribute.h DateTimeAttribute.h
libarcattr_la_SOURCES=RequestAttribute.cpp ArcAttributeFactory.cpp StringAttribute.cpp DateTimeAttribute.cpp
libarcattr_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs

