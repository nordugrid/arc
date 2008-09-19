javalibdir = $(libdir)/java
javasharedir = $(prefix)/share/java

CLEANFILES = *.class
javashare_DATA = EchoService.class

EXTRA_DISTS = EchoService.class EchoService.java compile.sh

EchoService.class: EchoService.java compile.sh
	./compile.sh
