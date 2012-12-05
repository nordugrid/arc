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
if XROOTD_ENABLED
XROOTD_SD=xrootd
endif
if GFAL_ENABLED
GFAL_SD=gfal
endif

SUBDIRS = mock http file arc $(LDAP_SD) $(GRIDFTP_SD) $(RLS_SD) $(LFC_SD) $(SRM_SD) $(XROOTD_SD) $(GFAL_SD)
DIST_SUBDIRS = mock http file arc ldap gridftp rls lfc srm xrootd gfal
