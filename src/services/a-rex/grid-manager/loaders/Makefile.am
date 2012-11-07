pkglibexec_PROGRAMS = downloader uploader

downloader_SOURCES = downloader.cpp
downloader_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
downloader_LDADD = \
	../conf/libconf.la ../jobs/libjobs.la ../log/liblog.la \
	../files/libfiles.la ../run/librun.la ../misc/libmisc.la \
	../mail/libmail.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la

uploader_SOURCES = uploader.cpp
uploader_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
uploader_LDADD = \
	../conf/libconf.la ../jobs/libjobs.la ../log/liblog.la \
	../files/libfiles.la ../run/librun.la ../misc/libmisc.la \
	../mail/libmail.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la
