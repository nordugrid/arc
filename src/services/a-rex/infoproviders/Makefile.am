pkgdata_SCRIPTS = CEinfo.pl cluster.pl qju.pl se.pl glue-generator.pl \
	grid-info-soft-register glite-info-provider-ldap

pkgdata_DATA = ARC0mod.pm SGEmod.pm FORKmod.pm PBS.pm DGBridge.pm \
	LL.pm LSF.pm Condor.pm condor_env.pm SLURM.pm GridFactory.pm \
	IniParser.pm LogUtils.pm Sysinfo.pm \
	LRMSInfo.pm GMJobsInfo.pm HostInfo.pm RTEInfo.pm \
	InfoChecker.pm ConfigCentral.pm \
	ARC0ClusterInfo.pm ARC1ClusterInfo.pm \
	SGE.pm Fork.pm LRMS.pm Shared.pm \
	XmlPrinter.pm GLUE2xmlPrinter.pm \
	LdifPrinter.pm GLUE2ldifPrinter.pm \
	NGldifPrinter.pm InfosysHelper.pm

arcldapschemadir = $(pkgdatadir)/ldap-schema
arcldapschema_DATA = schema/nordugrid.schema

EXTRA_DIST = $(pkgdata_SCRIPTS) $(pkgdata_DATA) $(arcldapschema_DATA)

SUBDIRS = test
