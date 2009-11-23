# Makefile to create the perl module subfolder janitor/Catalog inside the share folder

# share/ enthï¿½lt Dateien, die nicht architekturspezifisch sind
data_janitordir = $(pkgdatadir)/perl/Janitor/Catalog
dist_data_janitor_DATA = BaseSystem.pm DebianPackage.pm MetaPackageCollection.pm MetaPackage.pm NoteCollection.pm Note.pm Package.pm Tag.pm TarPackage.pm

JanitorPerlModuleManSection=8pm

Janitor\:\:BaseSystem.$(JanitorPerlModuleManSection): BaseSystem.pm
	pod2man $< > $@

Janitor\:\:DebianPackage.$(JanitorPerlModuleManSection): DebianPackage.pm
	pod2man $< > $@

Janitor\:\:MetaPackageCollection.$(JanitorPerlModuleManSection): MetaPackageCollection.pm
	pod2man $< > $@

Janitor\:\:MetaPackage.$(JanitorPerlModuleManSection): MetaPackage.pm
	pod2man $< > $@

Janitor\:\:NoteCollection.$(JanitorPerlModuleManSection): NoteCollection.pm
	pod2man $< > $@

Janitor\:\:Note.$(JanitorPerlModuleManSection): Note.pm
	pod2man $< > $@

Janitor\:\:MetaPackage.$(JanitorPerlModuleManSection): MetaPackage.pm
	pod2man $< > $@

Janitor\:\:Package.$(JanitorPerlModuleManSection): Package.pm
	pod2man $< > $@

Janitor\:\:Tag.$(JanitorPerlModuleManSection): Tag.pm
	pod2man $< > $@

Janitor\:\:TarPackage.$(JanitorPerlModuleManSection): TarPackage.pm
	pod2man $< > $@

man_MANS=Janitor\:\:BaseSystem.$(JanitorPerlModuleManSection) Janitor\:\:DebianPackage.$(JanitorPerlModuleManSection) Janitor\:\:MetaPackageCollection.$(JanitorPerlModuleManSection) Janitor\:\:MetaPackage.$(JanitorPerlModuleManSection) Janitor\:\:NoteCollection.$(JanitorPerlModuleManSection) Janitor\:\:Note.$(JanitorPerlModuleManSection) Janitor\:\:MetaPackage.$(JanitorPerlModuleManSection) Janitor\:\:Package.$(JanitorPerlModuleManSection) Janitor\:\:Tag.$(JanitorPerlModuleManSection) Janitor\:\:TarPackage.$(JanitorPerlModuleManSection)
