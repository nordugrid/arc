pkglibexec_SCRIPTS = CEinfo.pl cluster.pl qju.pl se.pl grid-info-soft-register

pkglib_DATA = ARC0mod.pm SGEmod.pm FORKmod.pm PBS.pm \
		  LL.pm LSF.pm Condor.pm condor_env.pm SLURM.pm GridFactory.pm \
		  IniParser.pm LogUtils.pm Sysinfo.pm \
		  LRMSInfo.pm GMJobsInfo.pm HostInfo.pm RTEInfo.pm \
		  InfoChecker.pm ConfigCentral.pm \
		  ARC0ClusterSchema.pm ARC0ClusterInfo.pm \
		  ARC1ClusterSchema.pm ARC1ClusterInfo.pm \
		  SGE.pm Fork.pm LRMS.pm Shared.pm glue-generator.pl

arcldapschemadir = $(pkgdatadir)/ldap-schema
arcldapschema_DATA = schema/nordugrid.schema

arcglueldapschemadir = $(arcldapschemadir)/glue-1.2
arcglueldapschema_DATA = schema/glue-1.2/Glue-CE.schema \
    schema/glue-1.2/Glue-CESEBind.schema \
    schema/glue-1.2/Glue-CORE.schema \
    schema/glue-1.2/Glue-MDS.schema \
    schema/glue-1.2/Glue-SE.schema

EXTRA_DIST = $(pkglibexec_SCRIPTS) $(pkglib_DATA) $(arcldapschema_DATA) $(arcglueldapschema_DATA)

SUBDIRS = test
