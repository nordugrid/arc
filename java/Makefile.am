JAVA_PATH = nordugrid/arc

javalibdir = $(libdir)/java
javasharedir = $(libdir)/java

javalib_LTLIBRARIES = libjarc.la
javashare_DATA = arc.jar

libjarc_la_SOURCES = arc_init.cpp arc_wrap.cpp arc_jni.h
libjarc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(JDK_CFLAGS) \
	-fno-strict-aliasing
libjarc_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libjarc_la_LDFLAGS = -no-undefined -avoid-version -module

SWIG_OUTPUTS = arc_wrap.cpp $(JAVA_PATH)/*.java

EXTRA_DIST = $(SWIG_OUTPUTS)

CLEANFILES = $(JAVA_PATH)/*.class arc.jar
MAINTAINERCLEANFILES = $(SWIG_OUTPUTS)

@AMDEP_TRUE@include ./$(DEPDIR)/arc_wrap.deps

arc_wrap.cpp: $(top_srcdir)/swig/*.i
	mkdir -p $(DEPDIR)
	grep -h '^#' $(top_srcdir)/swig/*.i | \
	$(CXXCOMPILE) $(libjarc_la_CXXFLAGS) -M -MT arc_wrap.cpp -MT $(JAVA_PATH)/arc.java -MP -MF "$(DEPDIR)/arc_wrap.deps" -x c++ -
	rm -rf $(JAVA_PATH)
	mkdir -p $(JAVA_PATH)
	$(SWIG) -v -c++ -java -nodefaultctor -nodefaultdtor -module arc \
		-package nordugrid.arc -o arc_wrap.cpp \
		-outdir $(JAVA_PATH) \
		-I/usr/include -I$(top_srcdir)/include \
                $(OPENSSL_CFLAGS) $(top_srcdir)/swig/Arc.i
	sed 's%#include <jni.h>%#include <arc_jni.h>%' < \
	arc_wrap.cpp > arc_wrap.cpp.tmp
	mv arc_wrap.cpp.tmp arc_wrap.cpp
	for javafile in `ls $(JAVA_PATH)/*.java` ; do \
		echo >> $(DEPDIR)/arc_wrap.deps ; \
		echo `sed s/.java$$/.class/ <<< $${javafile}`: $${javafile} \
		>> $(DEPDIR)/arc_wrap.deps ; \
		echo >> $(DEPDIR)/arc_wrap.deps ; \
		echo arc.jar: `sed s/.java$$/.class/ <<< $${javafile}` \
		>> $(DEPDIR)/arc_wrap.deps ; \
	done

$(JAVA_PATH)/*.java: arc_wrap.cpp

$(JAVA_PATH)/arc.class: $(JAVA_PATH)/arc.java

.java.class:
	$(JAVAC) $(JAVAC_FLAGS) $<

arc.jar: $(JAVA_PATH)/arc.class $(subst .java,.class,$(wildcard $(JAVA_PATH)/*.java))
	rm -f $@
	$(JAR) cf $@ $(JAVA_PATH)/*.class

# Try not to build these objects in parallel in order to save memory
.NOTPARALLEL: %.o %.class %.so
