if JAVA_ENABLED 
JAVA_SERVICES = echo_java 
else
JAVA_SERVICES =
endif

if PYTHON_ENABLED 
PYTHON_SERVICES = echo_python
else
PYTHON_SERVICES =
endif

if A_REX_SERVICE_ENABLED
AREX_SERVICE = a-rex
else
AREX_SERVICE =
endif

SUBDIRS=$(JAVA_SERVICES) $(PYTHON_SERVICES) $(AREX_SERVICE) pdp httpd arex2
DIST_SUBDIRS = echo_java echo_python a-rex pdp httpd arex2
