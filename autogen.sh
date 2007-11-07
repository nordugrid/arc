#!/bin/sh
#
# autogen.sh glue
#
# Requires: automake 1.9, autoconf 2.57+
# Conflicts: autoconf 2.13
set -x 

cleanup() {
    find . -type d -name autom4te.cache -print0 | xargs -0 rm -rf \;
    find . -type f \( -name missing -o -name install-sh \
        -o -name mkinstalldirs \
        -o -name depcomp -o -name ltmain.sh -o -name configure \
        -o -name config.sub -o -name config.guess \
        -o -name Makefile.in -o -name config.h.in -o -name aclocal.m4 \
        -o -name autoscan.log -o -name configure.scan -o -name config.log \
        -o -name config.status -o -name config.h -o -name stamp-h1 \
        -o -name Makefile -o -name libtool \) \
        -print0 | xargs -0 rm -f
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

# For the Debian package build
test -d debian_dummy && {
	# link these in Debian builds
	rm -f config.sub config.guess
	ln -s /usr/share/misc/config.sub .
	ln -s /usr/share/misc/config.guess .

	# refresh list of executable scripts, to avoid possible breakage if
	# upstream tarball does not include the file or if it is mispackaged
	# for whatever reason.
	[ "$1" == "updateexec" ] && {
		echo Generating list of executable files...
		rm -f debian/executable.files
		find . -type f -perm +111 ! -name '.*' -fprint debian/executable.files
	}

	# Remove any files in upstream tarball that we don't have in the Debian
	# package (because diff cannot remove files)
	version=`dpkg-parsechangelog | awk '/Version:/ { print $2 }' | sed -e 's/-[^-]\+$//'`
	source=`dpkg-parsechangelog | awk '/Source:/ { print $2 }' | tr -d ' '`
	if test -r ../${source}_${version}.orig.tar.gz ; then
		echo Generating list of files that should be removed...
		rm -f debian/deletable.files
		touch debian/deletable.files
		[ -e debian/tmp ] && rm -rf debian/tmp
		mkdir debian/tmp
		( cd debian/tmp ; tar -zxf ../../../${source}_${version}.orig.tar.gz )
		find debian/tmp/ -type f ! -name '.*' -print0 | xargs -0 -ri echo '{}' | \
		  while read -r i ; do
			if test -e "${i}" ; then
				filename=$(echo "${i}" | sed -e 's#.*debian/tmp/[^/]\+/##')
				test -e "${filename}" || echo "${filename}" >>debian/deletable.files
			fi
		  done
		rm -fr debian/tmp
	else
		echo Emptying list of files that should be deleted...
		rm -f debian/deletable.files
		touch debian/deletable.files
	fi
}

exit 0
