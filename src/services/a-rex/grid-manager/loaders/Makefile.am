pkglibexec_PROGRAMS=downloader uploader

downloader_SOURCES=downloader.cpp
downloader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
downloader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la ../jobs/libjobs.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la $(top_srcdir)/src/hed/acc/ARC1/libaccARC1.la

uploader_SOURCES=uploader.cpp
uploader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
uploader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la ../jobs/libjobs.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la

