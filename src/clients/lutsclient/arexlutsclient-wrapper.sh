#!/bin/sh

# Wrapper script for Accounting agent (A-REX Lutsclient)
#
# Please insert a line similar to the following
# into A-REX configuration file under section [gridmanager]:
#
#  helper=" . /opt/arc1/bin/arexlutsclient-wrapper.sh 86400 /etc/arexlutsclient.xml"
#
# You should replace the path with actual ARC installation path.
# First argument (here 86400) is the periodicity in seconds.
# Second argument is the client configuration XML file.

AREXLUTSCLIENT_EXECUTABLE=/opt/arc1/bin/arexlutsclient
AREXLUTSCLIENT_LOGFILE=/tmp/arexlutsclient.log

PERIOD=$1
shift

$AREXLUTSCLIENT_EXECUTABLE $@ >>$AREXLUTSCLIENT_LOGFILE 2>&1
sleep $PERIOD
