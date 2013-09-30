JAVA_PATH = nordugrid/arc

JAVA_INTERFACE_SRC = EndpointConsumer.java JobConsumer.java ComputingServiceTypeConsumer.java
JAVA_BUILD_PATH = $(top_builddir)/java/$(JAVA_PATH)/
JAVA_INTERFACE_SRC_MOVED = $(addprefix $(JAVA_BUILD_PATH), $(JAVA_INTERFACE_SRC))

jninative_LTLIBRARIES = libjarc.la
jni_DATA = arc.jar

if MACOSX
SHREXT = -shrext .dylib
else
SHREXT =
endif

if JAVA_IS_15_OR_ABOVE
SWIG_IS_JAVA_15_OR_ABOVE = -DJAVA_IS_15_OR_ABOVE
else
SWIG_IS_JAVA_15_OR_ABOVE = -DJAVA_IS_NOT_15_OR_ABOVE
endif

if DBJSTORE_ENABLED
SWIG_IS_DBJSTORE_ENABLED = -DDBJSTORE_ENABLED
else
SWIG_IS_DBJSTORE_ENABLED =
endif

libjarc_la_SOURCES = arc_init.cpp arc_jni.h
nodist_libjarc_la_SOURCES = arc_wrap.cpp
libjarc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(JDK_CFLAGS) $(ZLIB_CFLAGS) $(DBCXX_CPPFLAGS) \
	-fno-strict-aliasing $(AM_CXXFLAGS)
libjarc_la_LIBADD = \
	$(top_builddir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/libs/data-staging/libarcdatastaging.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(ZLIB_LIBS) $(DBCXX_LIBS)
libjarc_la_LDFLAGS = -no-undefined -avoid-version -module $(SHREXT)

SWIG_OUTPUTS = arc_wrap.cpp arc_wrap.h $(JAVA_PATH)/*.java

CLEANFILES = $(SWIG_OUTPUTS) $(JAVA_PATH)/*.class arc.jar

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
		$(SWIG_IS_JAVA_15_OR_ABOVE) $(SWIG_IS_DBJSTORE_ENABLED) \
		-I/usr/include -I$(top_srcdir)/include \
                $(OPENSSL_CFLAGS) $(top_srcdir)/swig/Arc.i
	sed 's%#include <jni.h>%#include <arc_jni.h>%' < \
	arc_wrap.cpp > arc_wrap.cpp.tmp
	mv arc_wrap.cpp.tmp arc_wrap.cpp
# When mapping a template with a template class argument no space is
# inserted between the two right angle brackets.
	sed 's/>>(new/> >(new/g' arc_wrap.cpp > arc_wrap.cpp.tmp
	mv arc_wrap.cpp.tmp arc_wrap.cpp 
# When mapping a template with a template class argument no space is
# inserted between the two right angle brackets.
	sed 's/>>(self->/> >(self->/g' arc_wrap.cpp > arc_wrap.cpp.tmp
	mv arc_wrap.cpp.tmp arc_wrap.cpp 
# When mapping a template with another template class as argument, and
# that template class takes two classes as argument, then older swigs
# put parentheses around the two class arguments, e.g. T<(A,B)>, not
# valid syntax should be T<A,B> instead.
	sed 's/<(\([,:[:alnum:]]*\))>/<\1>/g' arc_wrap.cpp > arc_wrap.cpp.tmp 
	mv arc_wrap.cpp.tmp arc_wrap.cpp
	for javafile in `ls $(JAVA_PATH)/*.java` ; do \
		echo >> $(DEPDIR)/arc_wrap.deps ; \
		echo `sed s/.java$$/.class/ <<< $${javafile}`: $${javafile} \
		>> $(DEPDIR)/arc_wrap.deps ; \
		echo >> $(DEPDIR)/arc_wrap.deps ; \
		echo arc.jar: `sed s/.java$$/.class/ <<< $${javafile}` \
		>> $(DEPDIR)/arc_wrap.deps ; \
	done


$(JAVA_INTERFACE_SRC_MOVED): $(JAVA_BUILD_PATH)% : $(JAVA_INTERFACE_SRC)
	cp -p $(top_srcdir)/java/$* $@

$(JAVA_PATH)/*.java: arc_wrap.cpp $(JAVA_INTERFACE_SRC_MOVED)

$(JAVA_PATH)/arc.class: $(JAVA_PATH)/arc.java $(JAVA_INTERFACE_SRC_MOVED)

.java.class: $(JAVA_INTERFACE_SRC_MOVED)
	$(JAVAC) $(JAVAC_FLAGS) $<

arc.jar: $(JAVA_PATH)/arc.class $(subst .java,.class,$(wildcard $(JAVA_PATH)/*.java))
	rm -f $@
	$(JAR) cf $@ $(JAVA_PATH)/*.class $(JAR_JFLAGS)

DIST_SUBDIRS = test examples
SUBDIRS = $(TEST_DIR) examples

EXTRA_DIST = $(JAVA_INTERFACE_SRC)

# Try not to build these objects in parallel in order to save memory
.NOTPARALLEL: %.o %.class %.so
