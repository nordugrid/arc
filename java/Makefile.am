# Define java Package directory, the tailing '/' is mandatory.a
JAVA_PATH = org/nordugrid/arc/
JDK_INCLUDES = @JDK_INCLUDE@ -I$(top_builddir)
CLASSPATH = .

javalibdir = $(prefix)/lib/java
javasharedir = $(prefix)/share/java

# Shared library.
#if MINGW
#javalib_LTLIBRARIES = jarc.la
#else
javalib_LTLIBRARIES = libjarc.la
#endif

#jarc_la_SOURCES =	arc_wrap.c
#jarc_la_CFLAGS = $(JDK_INCLUDES) -I$(top_srcdir)
## jarc_la_LIBADD = $(top_builddir)/lasso/liblasso.la $(LASSO_LIBS)
#jarc_la_LDFLAGS =	-no-undefined -avoid-version -module \
#                        -Wl,--add-stdcall-alias 

libjarc_la_SOURCES =	arc_wrap.cpp
libjarc_la_CPPFLAGS = $(JDK_INCLUDES) -I$(top_srcdir) -I$(top_srcdir)/src/libs/ -I$(top_srcdir)/src/hed/libs $(LIBXML2_CFLAGS)
libjarc_la_LIBADD =	$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
libjarc_la_LDFLAGS =	-no-undefined -avoid-version -module

javashare_DATA = arc.jar

SWIG_OUTPUTS = arc_wrap.cpp *.java 

JAVA_CRUFT = $(JAVA_PATH)arc.class arc_wrap.cpp arc_wrap.o arc.jar $(JAVA_PATH)arc.java

# The SWIG outputs are included in the distribution so SWIG is not
# required to use the Java interface
EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = $(JAVA_CRUFT) 
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

SWIG_I_FILES=$(shell find $(top_srcdir)/swig/ -name '*.[ih]')
arc_wrap.cpp $(JAVA_PATH)arc.java: $(SWIG_I_FILES)
	mkdir -p $(JAVA_PATH)
	$(SWIG) -v -c++ -java -module arc -package org.nordugrid.arc -o arc_wrap.cpp -outdir $(JAVA_PATH) \
		$(top_srcdir)/swig/Arc.i

clean-local:
	rm -f *.class
	rm -rf org

all-local: arc.jar

$(JAVA_PATH)arc.class: $(JAVA_PATH)arc.java
	$(JAVAC) $(JAVAC_FLAGS) $(JAVA_PATH)*.java

arc.jar: $(JAVA_PATH)arc.class 
	rm -f $@
	$(JAR) cf $@ $(JAVA_PATH)*.class 

