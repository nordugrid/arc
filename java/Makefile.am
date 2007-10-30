JAVA_PATH = nordugrid/arc

javalibdir = $(libdir)/java
javasharedir = $(prefix)/share/java

javalib_LTLIBRARIES = libjarc.la
javashare_DATA = arc.jar

libjarc_la_SOURCES = arc_wrap.cpp
libjarc_la_CXXFLAGS = -fno-strict-aliasing $(JDK_CFLAGS) -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -DJNIH='$(JNIH)'
libjarc_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libjarc_la_LDFLAGS = -no-undefined -avoid-version -module

SWIG_OUTPUTS = arc_wrap.cpp $(JAVA_PATH)/*.java

EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = $(JAVA_PATH)/*.class arc.jar
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

include ./$(DEPDIR)/arc_wrap.deps

arc_wrap.cpp $(JAVA_PATH)/arc.java $(JAVA_PATH)/%.java: $(top_srcdir)/swig/*.i
	grep -h '^#' $(top_srcdir)/swig/*.i | \
	$(CXXCOMPILE) $(libjarc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT $(JAVA_PATH)/arc.java -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	mkdir -p $(JAVA_PATH)
	$(SWIG) -v -c++ -java -nodefault -module arc \
		-package nordugrid.arc -o arc_wrap.cpp \
		-outdir $(JAVA_PATH) \
		-I$(top_srcdir)/include $(top_srcdir)/swig/Arc.i
	sed 's%#include <jni.h>%#include JNIH%' < \
	arc_wrap.cpp > arc_wrap.cpp.tmp
	mv arc_wrap.cpp.tmp arc_wrap.cpp

$(JAVA_PATH)/arc.class $(JAVA_PATH)/%.class: $(JAVA_PATH)/arc.java
	$(JAVAC) $(JAVAC_FLAGS) $(JAVA_PATH)/*.java

arc.jar: $(JAVA_PATH)/arc.class $(patsubst %.java,%.class,$(shell ls $(JAVA_PATH)/*.java))
	rm -f $@
	$(JAR) cf $@ $(JAVA_PATH)/*.class 
