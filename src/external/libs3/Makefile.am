noinst_LTLIBRARIES = libs3.la

libs3_la_SOURCES = src/acl.c src/bucket.c src/error_parser.c src/general.c \
                 src/object.c src/request.c src/request_context.c \
                 src/response_headers_handler.c src/service_access_logging.c \
                 src/service.c src/simplexml.c src/util.c src/s3.c \
                 inc/error_parser.h inc/libs3.h inc/request_context.h \
                 inc/request.h inc/response_headers_handler.h \
                 inc/simplexml.h inc/string_buffer.h inc/util.h
libs3_la_CFLAGS = -I$(top_srcdir)/src/external/libs3/inc \
           $(LIBXML2_CFLAGS)\
          -DLIBS3_VER_MAJOR=\"2\"\
          -DLIBS3_VER_MINOR=\"0\"\
          -DLIBS3_VER=\"2.0\"
libs3_la_LIBADD = -lm -lpthread $(CURL_LIBS)  $(LIBXML2_LIBS) -lz 
