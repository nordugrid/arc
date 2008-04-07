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

SUBDIRS=$(AREX_SERVICE) $(ISI_SERVICE) pdp httpd arex2 wrappers paul sched
DIST_SUBDIRS = echo_java echo_python a-rex isis pdp httpd arex2 sched paul wrappers
