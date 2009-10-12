#!/bin/sh
#
# autogen.sh glue
#
# Requires: automake 1.9, autoconf 2.57+
# Conflicts: autoconf 2.13
set -x 

cleanup() {
    find . -type d -name autom4te.cache -print | xargs rm -rf \;
    find . -type f \( -name missing -o -name install-sh \
        -o -name mkinstalldirs \
        -o -name depcomp -o -name ltmain.sh -o -name configure \
        -o -name config.sub -o -name config.guess \
        -o -name Makefile.in -o -name config.h.in -o -name aclocal.m4 \
        -o -name autoscan.log -o -name configure.scan -o -name config.log \
        -o -name config.status -o -name config.h -o -name stamp-h1 \
        -o -name Makefile -o -name libtool \) \
        -print | xargs rm -f
}

if [ "x$1" = "xclean" ]; then
    cleanup
    exit
fi

# Refresh GNU autotools toolchain.
echo Cleaning autotools files...
cleanup

type glibtoolize > /dev/null 2>&1 && export LIBTOOLIZE=glibtoolize

echo Running autoreconf...
autoreconf --verbose --force --install

exit 0
