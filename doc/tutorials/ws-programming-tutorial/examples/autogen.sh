#!/bin/sh


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


touch NEWS AUTHORS README ChangeLog

# Refresh GNU autotools toolchain.
echo Cleaning autotools files...
cleanup

type glibtoolize > /dev/null 2>&1 && export LIBTOOLIZE=glibtoolize

# aclocal --force -I m4

echo Running autoreconf...
autoreconf --verbose --force --install




