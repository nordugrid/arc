# Makefile for the whole janitor service (.)

# install perl module
SUBDIRS = module interfaces schema


#
# Installation strategy:
#
# * The main source code is deployed in the directory "module". 
#   For it is structed similar to real perl modules, it is shiped to: $(pkgdatadir)/perl/Janitor
#
# * The module is very generic and can be accessed by application interface or (in future versions)
#   via a webservice. Due to the various ways to interface Janitor, there is an own directory for all
#   interfaces. They are shipped to: $(pkgdatadir)/perl/Janitor
#
# * The main interface for Janitor is the COMMANDLINE interface which takes a certain position. 
#   In the current version of Janitor, it has to be executed as root. For this a suid root helper
#   is needed called rjanitor.c which is compiled in the commandline interface directory 
#   A LINK will be created in the libexec directory pointing to this executable. 
#
