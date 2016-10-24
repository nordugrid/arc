#!/bin/bash -x

# This is not an actual script. It's just a safe example script how to use
# PerfData.pl inside a bash script.

# This line below can be integrated in an external script to call the 
# Infoproviders's ARC performance writer PerfData.pl.
# must be run inside the infoprovider modules folder, usually /usr/share/arc/
# Reads configuration options from arc.conf and finds the folder 
# that contains the NYTProf database files, converts whatever is
# inside to the ARC performance format and writes it into perfdata/infosys.perflog
# without the --keepnytproffiles flag it also deletes the NYTProf databases.
# If you don't want to 
# keep the source NYTProf database files, remove the flag '--keepnytproffiles'
# Use 'PerfData.pl --help' to get more info.
# The line below is the common usage for most cases.

cd /usr/share/arc/
PerfData.pl --config /etc/arc.conf --keepnytproffiles

## this method below is obsolete and not needed anymore. It is kept here
## in case one wants to summarize data in bigger files but disabled for now.
#
#echo "Usage: $0 <arc performance data path>"
#echo "default: $0 /var/log/arc/perfdata/"
#
#PERFDIR=${1:-/var/log/arc/perfdata/}
#MERGEDATE=`date +%Y%m%d%H%M%S`
#
#nytprofmerge -o infosys_$MERGEDATE.perflog $PERFDIR/infosys_*.raw
