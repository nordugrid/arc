if RLS_ENABLED 
RLS_SD = rls
endif
if LFC_ENABLED 
LFC_SD = lfc
endif
if GRIDFTP_ENABLED
GRIDFTP_SD = gridftp
endif
if LDAP_ENABLED
LDAP_SD = ldap
endif
if SRM_DMC_ENABLED
SRM_SD=srm
endif

SUBDIRS = http file $(LDAP_SD) $(GRIDFTP_SD) $(RLS_SD) $(LFC_SD) $(SRM_SD)
DIST_SUBDIRS = http file ldap gridftp rls lfc srm
