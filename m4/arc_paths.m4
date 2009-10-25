dnl
dnl Substitite some relative paths
dnl

AC_DEFUN([ARC_RELATIVE_PATHS],
[
  AC_REQUIRE([ARC_RELATIVE_PATHS_INIT])
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])

  AC_LIB_WITH_FINAL_PREFIX([
      eval instprefix="\"${exec_prefix}\""
      eval arc_libdir="\"${libdir}\""
      eval arc_bindir="\"${bindir}\""
      eval arc_sbindir="\"${sbindir}\""
      eval arc_pkglibdir="\"${libdir}/${PACKAGE}\""
      eval arc_pkglibexecdir="\"${libexecdir}/${PACKAGE}\""
      # It seems arc_datadir should be evaluated twice to be expanded fully.
      eval arc_datadir="\"${datadir}/${PACKAGE}\""
      eval arc_datadir="\"${arc_datadir}\""
      eval arc_sysconfdir="\"${sysconfdir}/${PACKAGE}\""
  ])

  libsubdir=`get_relative_path "$instprefix" "$arc_libdir"`
  pkglibsubdir=`get_relative_path "$instprefix" "$arc_pkglibdir"`
  pkglibexecsubdir=`get_relative_path "$instprefix" "$arc_pkglibexecdir"`
  pkglibdir_rel_to_pkglibexecdir=`get_relative_path "$arc_pkglibexecdir" "$arc_pkglibdir"`
  sbindir_rel_to_pkglibexecdir=`get_relative_path "$arc_pkglibexecdir" "$arc_sbindir"`
  bindir_rel_to_pkglibexecdir=`get_relative_path "$arc_pkglibexecdir" "$arc_bindir"`

  AC_MSG_NOTICE([pkglib subdirectory is: $pkglibsubdir])
  AC_MSG_NOTICE([pkglibexec subdirectory is: $pkglibexecsubdir])
  AC_MSG_NOTICE([relative path of pkglib to pkglibexec is: $pkglibdir_rel_to_pkglibexecdir])

  AC_SUBST([libsubdir])
  AC_SUBST([pkglibsubdir])
  AC_SUBST([pkglibexecsubdir])
  AC_SUBST([pkglibdir_rel_to_pkglibexecdir])
  AC_SUBST([sbindir_rel_to_pkglibexecdir])
  AC_SUBST([bindir_rel_to_pkglibexecdir])

  AC_DEFINE_UNQUOTED([INSTPREFIX], ["${instprefix}"], [installation prefix])
  AC_DEFINE_UNQUOTED([LIBSUBDIR], ["${libsubdir}"], [library installation subdirectory])
  AC_DEFINE_UNQUOTED([PKGLIBSUBDIR], ["${pkglibsubdir}"], [plugin installation subdirectory])
  AC_DEFINE_UNQUOTED([PKGLIBEXECSUBDIR], ["${pkglibexecsubdir}"], [helper programs installation subdirectory])
  AC_DEFINE_UNQUOTED([PKGDATADIR], ["${arc_datadir}"], [arc data dir])
  AC_DEFINE_UNQUOTED([PKGSYSCONFDIR], ["${arc_sysconfdir}"], [arc system configuration directory])

])

AC_DEFUN([ARC_RELATIVE_PATHS_INIT],
[
  get_relative_path() {
    olddir=`echo $[]1 | sed -e 's|/+|/|g' -e 's|^/||' -e 's|/*$|/|'`
    newdir=`echo $[]2 | sed -e 's|/+|/|g' -e 's|^/||' -e 's|/*$|/|'`

    O_IFS=$IFS
    IFS=/
    relative=""
    common=""
    for i in $olddir; do
      if echo "$newdir" | grep -q "^$common$i/"; then
        common="$common$i/"
      else
        relative="../$relative"
      fi
    done
    IFS=$O_IFS
    echo $newdir | sed "s|^$common|$relative|" | sed 's|/*$||'
  }
])

