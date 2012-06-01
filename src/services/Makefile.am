if A_REX_SERVICE_ENABLED
AREX_SERVICE = a-rex
else
AREX_SERVICE =
endif

if GRIDFTPD_SERVICE_ENABLED
GRIDFTPD_SERVICE = gridftpd
else
GRIDFTPD_SERVICE =
endif

if LDAP_SERVICE_ENABLED
LDAP_SERVICE = ldap-infosys
else
LDAP_SERVICE =
endif

if LDAP_MONITOR_ENABLED
LDAP_MONITOR = ldap-monitor
else
LDAP_MONITOR =
endif

if WS_MONITOR_ENABLED
WS_MONITOR = ws-monitor
else
WS_MONITOR =
endif

if PYTHON_SERVICE
PYTHON_SERVICE = echo_python
else
PYTHON_SERVICE =
endif

#if JAVA_ENABLED
#JAVA_SERVICE = echo_java
#else
JAVA_SERVICE =
#endif

if CACHE_SERVICE_ENABLED
CACHE_SERVICE = cache_service
else
CACHE_SERVICE =
endif

if DATADELIVERY_SERVICE_ENABLED
DATADELIVERY_SERVICE = data-staging
else
DATADELIVERY_SERVICE =
endif

SUBDIRS = $(GRIDFTPD_SERVICE) $(AREX_SERVICE) $(LDAP_SERVICE) \
	$(LDAP_MONITOR) $(WS_MONITOR) \
	$(CACHE_SERVICE) \
	$(DATADELIVERY_SERVICE) wrappers $(PYTHON_SERVICE) $(JAVA_SERVICE)
DIST_SUBDIRS = gridftpd a-rex ldap-infosys \
	ldap-monitor ws-monitor \
	cache_service \
	data-staging wrappers echo_python echo_java
