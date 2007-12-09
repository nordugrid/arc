libexec_PROGRAMS=downloader 

downloader_SOURCES=downloader.cpp
downloader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
downloader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la ../jobs/libjobs.la ../cache/libcache.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la
