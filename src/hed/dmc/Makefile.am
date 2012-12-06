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
if MOCK_DMC_ENABLED
MOCK_SD=mock
endif

SUBDIRS = http file arc $(LDAP_SD) $(GRIDFTP_SD) $(RLS_SD) $(LFC_SD) $(SRM_SD) $(XROOTD_SD) $(GFAL_SD) $(MOCK_SD)
DIST_SUBDIRS = http file arc ldap gridftp rls lfc srm xrootd gfal mock
