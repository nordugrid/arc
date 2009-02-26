#!/bin/sh

# Wrapper script for Accounting agent (JARM)
#
# Please insert a line similar to the following
# into A-REX configuration file under section [gridmanager]:
#
#  helper=" . /opt/arc1/bin/arexjarm-wrapper.sh 86400 /etc/arexjarm.xml"
#
# You should replace the path with actual ARC installation path.
# First argument (here 86400) is the periodicity in seconds.
# Second argument is the client configuration XML file.

AREXJARM_EXECUTABLE=/opt/arc1/bin/arexjarm

PERIOD=$1
shift

$AREXJARM_EXECUTABLE $@
sleep $PERIOD
