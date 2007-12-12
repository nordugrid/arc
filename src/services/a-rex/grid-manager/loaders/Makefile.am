libexec_PROGRAMS=downloader uploader

downloader_SOURCES=downloader.cpp
downloader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
downloader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la ../jobs/libjobs.la ../cache/libcache.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la

uploader_SOURCES=uploader.cpp
uploader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
uploader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la ../jobs/libjobs.la ../cache/libcache.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la
