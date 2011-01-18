#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([nordugrid-arc-doc], [1.0.0], [http://bugzilla.nordugrid.org/])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
#AC_PROG_INSTALL

#If `ln -s' works on the current file system (the operating system and file system support symbolic links), set the output variable LN_S to `ln -s'; otherwise, if `ln' works, set LN_S to `ln', and otherwise set it to `cp -p'. 
AC_PROG_LN_S

# Checks for libraries.
AC_CHECK_PROGS(PDFLATEX, pdflatex)
#AC_CHECK_PROGS(DOT, dot)
if test "x$PDFLATEX" = "x"; then
  enable_doc="no"
  AC_MSG_NOTICE([WARNING: Missing pdflatex - documentation won't be built])
elif test "x$DOXYGEN" = "x"; then
  enable_doc="no"
#elif test "x$DOT" = "x"; then
#  enable_doc="no"
#  AC_MSG_NOTICE([WARNING: Missing dot - documentation won't be built])
fi



AC_SUBST(PDFLATEX)
AC_SUBST(DOXYGEN)

#AC_CONFIG_SUBDIR([ws-programming-tutorial])

AC_CONFIG_FILES([Makefile
                 nordugrid-arc-doc.spec
                 tech_doc/Makefile
                 tech_doc/client/Makefile
                 tech_doc/a-rex/Makefile
                 tech_doc/gridftpd/Makefile
                 tech_doc/hed/Makefile
                 tech_doc/infosys/Makefile
                 tech_doc/infosys/TechnicalHandbook/Makefile
                 tech_doc/backend_interface/Makefile
                 tech_doc/jura/Makefile
                 tech_doc/chelonia/Makefile
                 tech_doc/sec/Makefile
                 tech_doc/ldap-arc-infosys/Makefile
                 tech_doc/urlogger/Makefile
                 tech_doc/xrsl/Makefile
                 tutorials/Makefile
                 tutorials/ws-programming-tutorial/Makefile
                 tutorials/ws-programming-tutorial/doc/Makefile
                 manuals/Makefile
                 manuals/user_interface/Makefile
                 manuals/ws-quick-guide/Makefile
                 manuals/janitor/Makefile
                 manuals/configuration/Makefile
                 manuals/chelonia/Makefile
                 manuals/hopi/Makefile
                 manuals/echo/Makefile
                 manuals/python/Makefile
                 manuals/monitor/Makefile
                 release_notes/Makefile])

AC_OUTPUT
