# Makefile to create the perl module janitor inside the share folder


# share/ contains files that are not architecture-specific
data_janitordir = $(pkgdatadir)/perl/Janitor
dist_data_janitor_DATA = ArcConfig.pm Catalog.pm Common.pm Execute.pm Filefetcher.pm Installer.pm InstallLogger.pm Janitor.pm Job.pm Request.pm Response.pm RTE.pm TarPackageShared.pm Util.pm

JanitorPerlModuleManSection=8pm

Janitor\:\:Janitor.$(JanitorPerlModuleManSection): Janitor.pm
	pod2man $< > $@

Janitor\:\:Catalog.$(JanitorPerlModuleManSection): Catalog.pm
	pod2man $< > $@

Janitor\:\:Common.$(JanitorPerlModuleManSection): Common.pm
	pod2man $< > $@

Janitor\:\:Execute.$(JanitorPerlModuleManSection): Execute.pm
	pod2man $< > $@

Janitor\:\:Filefetcher.$(JanitorPerlModuleManSection): Filefetcher.pm
	pod2man $< > $@

Janitor\:\:Installer.$(JanitorPerlModuleManSection): Installer.pm
	pod2man $< > $@

Janitor\:\:InstallLogger.$(JanitorPerlModuleManSection): InstallLogger.pm
	pod2man $< > $@

Janitor\:\:Job.$(JanitorPerlModuleManSection): Job.pm
	pod2man $< > $@

Janitor\:\:Request.$(JanitorPerlModuleManSection): Request.pm
	pod2man $< > $@

Janitor\:\:Response.$(JanitorPerlModuleManSection): RTE.pm
	pod2man $< > $@

Janitor\:\:TarPackageShared.$(JanitorPerlModuleManSection): TarPackageShared.pm
	pod2man $< > $@

Janitor\:\:Util.$(JanitorPerlModuleManSection): Util.pm
	pod2man $< > $@

man_MANS=JanitorPerlModuleManSection=3arc Janitor\:\:Janitor.$(JanitorPerlModuleManSection) Janitor\:\:Catalog.$(JanitorPerlModuleManSection) Janitor\:\:Common.$(JanitorPerlModuleManSection) Janitor\:\:Execute.$(JanitorPerlModuleManSection) Janitor\:\:Filefetcher.$(JanitorPerlModuleManSection) Janitor\:\:Installer.$(JanitorPerlModuleManSection) Janitor\:\:InstallLogger.$(JanitorPerlModuleManSection) Janitor\:\:Job.$(JanitorPerlModuleManSection) Janitor\:\:Request.$(JanitorPerlModuleManSection) Janitor\:\:Response.$(JanitorPerlModuleManSection) Janitor\:\:TarPackageShared.$(JanitorPerlModuleManSection) Janitor\:\:Util.$(JanitorPerlModuleManSection)

SUBDIRS = Catalog
