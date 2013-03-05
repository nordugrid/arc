check_PROGRAMS = basic_job_submission

basic_job_submission_SOURCES = basic_job_submission.cpp
basic_job_submission_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
basic_job_submission_LDADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	../libarccompute.la $(GLIBMM_LIBS)

exampledir = $(pkgdatadir)/examples/sdk
example_DATA = basic_job_submission.cpp
