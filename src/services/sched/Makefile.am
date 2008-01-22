pkglib_LTLIBRARIES = libgrid_sched.la
libgrid_sched_la_SOURCES = grid_sched.cpp grid_sched.h job_desc.cpp job_desc.h job.h job.cpp job_sched_meta.cpp job_sched_meta.h job_queue.h job_queue.cpp csg.h eps.h resource.h resources_handling.h eps.cpp csg.cpp create_activity.cpp get_activity_documents.cpp get_activity_statuses.cpp terminate_activities.cpp stop_accepting_new_activities.cpp start_accepting_new_activities.cpp change_activity_status.cpp get_activity.cpp report_activity_status.cpp get_activity_status_changes.cpp
libgrid_sched_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgrid_sched_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libgrid_sched_la_LDFLAGS = -no-undefined -avoid-version -module
