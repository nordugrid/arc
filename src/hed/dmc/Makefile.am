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

SUBDIRS = http file srm $(LDAP_SD) $(GRIDFTP_SD) $(RLS_SD) $(LFC_SD)
DIST_SUBDIRS = http file ldap gridftp rls lfc srm
