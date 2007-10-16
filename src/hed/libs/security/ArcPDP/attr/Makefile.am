#plugin
pkglib_LTLIBRARIES = libarcattrplugin.la
libarcattrplugin_la_SOURCES = ArcAttributeFactory.cpp ArcAttributeFactory.h 
libarcattrplugin_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcattrplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libarcattr.la
libarcattrplugin_la_LDFLAGS = -no-undefined -avoid-version -module


#share library
lib_LTLIBRARIES = libarcattr.la
libarcattr_ladir = $(pkgincludedir)/security/ArcPDP/attr
libarcattr_la_HEADERS = AttributeValue.h AttributeProxy.h AttributeFactory.h RequestAttribute.h
libarcattr_la_SOURCES = RequestAttribute.cpp StringAttribute.cpp StringAttribute.h DateTimeAttribute.cpp DateTimeAttribute.h X500NameAttribute.cpp X500NameAttribute.h AnyURIAttribute.cpp AnyURIAttribute.h
libarcattr_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcattr_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la

