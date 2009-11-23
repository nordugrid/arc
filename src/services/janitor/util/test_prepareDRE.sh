#!/bin/sh

set -e

# Script to execute the prepareDRE.pl script.

echo
echo " *  Preparing simple dynamic Runtime Environment"

# preparing directory to transform into a dynamic RTE
d=$(mktemp --directory --tmpdir=. rawDirectory_XXX  )

cat <<EOAPPLICATION > "$d"/welcomeApp.sh
#!/bin/sh
echo Welcome $* to dynamic runtime environments
EOAPPLICATION

if [ -r re-simple.tar.gz ] ; then rm re-simple.tar.gz; fi
./prepareDRE.pl --re re-simple.tar.gz --catalog catalog-simple.rdf --dir "$d"

# cleaning up
if [ -n "$d" ]; then rm -r "$d"; fi

if [ ! -r re-simple.tar.gz ]; then
	echo "Could not find resulting dRE"
	exit -1
fi

echo
echo " *  Preparing slightly more complex Runtime Environment"

# preparing directory to transform into a dynamic RTE
d=$(mktemp --directory --tmpdir=. rawDirectory_XXX  )

mkdir "$d"/bin
mkdir "$d"/lib

cat <<EOAPPLICATION > "$d"/bin/welcomeApp.sh
#!/bin/sh
echo Welcome $* to dynamic runtime environments
EOAPPLICATION

if [ -r re-complex.tar.gz ] ; then rm re-complex.tar.gz; fi
./prepareDRE.pl --re re-complex.tar.gz --dir "$d"

# cleaning up
if [ -n "$d" ]; then rm -r "$d"; fi

if [ ! -r re-complex.tar.gz ]; then
	echo "Could not find resulting dRE"
	exit -1
fi
