lib_LTLIBRARIES = libarcattr.la

libarcattr_la_SOURCES = AttributeValue.h AttributeProxy.h AttributeFactory.h \
	RequestAttribute.cpp	RequestAttribute.h \
	ArcAttributeFactory.cpp	ArcAttributeFactory.h \
	StringAttribute.cpp	StringAttribute.h \
	DateTimeAttribute.cpp	DateTimeAttribute.h \
	X500NameAttribute.cpp	X500NameAttribute.h \
	AnyURIAttribute.cpp	AnyURIAttribute.h
libarcattr_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	-I$(top_srcdir)/include
libarcattr_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la

