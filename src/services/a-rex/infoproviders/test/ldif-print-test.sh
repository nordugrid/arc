#!/bin/bash

#if [ $UID != 0 ] 
#   then
#   	echo "Sorry, this script must be run as root."
#	exit 1
#fi

#echo "This script must NOT be run on a running cluster, as it might compromise"
#echo "infosys behaviour. Use with care!"

#read -p "Continue? y/n"
#if [ "$REPLY" != 'y' ] 
#   then
#   echo "Please execute script again and enter \"y\" if you really want to continue."
#   exit 0
#fi

# this script is intended to setup the needed
# for testing ldif printing.
# will assume /tmp/ as the base dir for BDII stuff.

# create fake bdii dirs
echo "creating fake bdii dirs..."
mkdir -p /tmp/run/arc/bdii
mkdir -p /tmp/tmp/arc/bdii
mkdir -p /tmp/run/arc/infosys 


# create fake bdii pidfile
echo "Creating fake bdii pidfile"
touch /tmp/bdii-update.pid

# create fake gridftpd pid file
echo "Creating fake gridftpd pidfile"
touch /tmp/gridftpd.pid

# create fake gridmanager pid file
echo "Creating fake gridftpd pidfile"
touch /tmp/grid-manager.pid

# chmod for normal user to run CEinfo.pl and read ldif-script.sh
echo "Chmodding dirs for normal user to access..."
chmod -R 777 /tmp/run/arc/bdii
chmod -R 777 /tmp/run/arc/infosys
chmod -R 777 /tmp/tmp/arc/bdii

echo "please see ldif-print-test-arc.conf and pass it to CEinfo.pl"
echo "AREX call: ./CEinfo.pl --splitjobs --config test/ldif-print-test-arc.conf"
echo "Simple execution that includes jobs in XML and LDIF: sudo ./CEinfo.pl --splitjobs --config test/ldif-print-test-arc.conf" 
