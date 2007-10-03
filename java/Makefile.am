JAVA_PATH = nordugrid/arc
CLASSPATH = .

javalibdir = $(libdir)/java
javasharedir = $(prefix)/share/java

# Shared library.
javalib_LTLIBRARIES = libjarc.la

libjarc_la_SOURCES = arc_wrap.cpp
libjarc_la_CXXFLAGS = -fno-strict-aliasing $(JDK_CFLAGS) -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libjarc_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
libjarc_la_LDFLAGS = -no-undefined -avoid-version -module

javashare_DATA = arc.jar

SWIG_OUTPUTS = arc_wrap.cpp $(JAVA_PATH)/*.java 

JAVA_CRUFT = $(JAVA_PATH)/arc.class arc_wrap.cpp arc_wrap.o arc.jar $(JAVA_PATH)/arc.java

# The SWIG outputs are included in the distribution so SWIG is not
# required to use the Java interface
EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = $(JAVA_CRUFT) 
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

SWIG_I_FILES=$(shell find $(top_srcdir)/swig/ -name '*.[ih]')
arc_wrap.cpp $(JAVA_PATH)/*.java: $(SWIG_I_FILES)
	../install-sh -d $(JAVA_PATH)
	$(SWIG) -v -c++ -java -nodefault -module arc \
		-package nordugrid.arc -o arc_wrap.cpp \
		-outdir $(JAVA_PATH) -D'__attribute__(x)=' \
		-I$(top_srcdir)/include $(top_srcdir)/swig/Arc.i

clean-local:
	rm -rf nordugrid

all-local: arc.jar

$(JAVA_PATH)/arc.class $(JAVA_PATH)/%.class: $(JAVA_PATH)/arc.java
	$(JAVAC) $(JAVAC_FLAGS) $(JAVA_PATH)/*.java

arc.jar: $(JAVA_PATH)/arc.class $(patsubst %.java,%.class,$(shell ls $(JAVA_PATH)/*.java))
	rm -f $@
	$(JAR) cf $@ $(JAVA_PATH)/*.class 
