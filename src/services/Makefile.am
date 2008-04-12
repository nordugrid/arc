if A_REX_SERVICE_ENABLED 
AREX_SERVICE = a-rex 
else 
AREX_SERVICE = 
endif 

if ISI_SERVICE_ENABLED 
ISI_SERVICE = isis 
else 
ISI_SERVICE = 
endif 

if PDP_SERVICE_ENABLED
PDP_SERVICE = pdp 
else 
PDP_SERVICE = 
endif 

if HTTPD_SERVICE_ENABLED
HTTPD_SERVICE = httpd 
else 
HTTPD_SERVICE = 
endif 

if SAML_ENABLED
SAML = saml
else
SAML =
endif

SUBDIRS=$(AREX_SERVICE) $(ISI_SERVICE) $(PDP_SERVICE) $(HTTPD_SERVICE) arex2 wrappers paul sched $(SAML)
DIST_SUBDIRS = echo_java echo_python a-rex isis pdp httpd arex2 sched paul wrappers saml
