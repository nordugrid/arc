#!/bin/bash

if [ $UID != 0 ] 
   then
   	echo "Sorry, this script must be run as root."
	exit 1
fi

echo "This script must NOT be run on a running cluster, as it might compromise"
echo "infosys behaviour. Use with care!"

read -p "Continue? y/n"
if [ "$REPLY" != 'y' ] 
   then
   echo "Please execute script again and enter \"y\" if you really want to continue."
   exit 0
fi

# this script is intended to setup the needed
# for testing ldif printing.
# will assume /tmp/ as the base dir for BDII stuff.

# create fake bdii dirs
mkdir -p /tmp/run/arc/bdii
mkdir -p /tmp/tmp/arc/bdii
# this is not relocatable. Needs to be root
mkdir -p /var/run/arc/infosys 


# create fake bdii pidfile
touch /tmp/bdii-update.pid

# chmod for normal user to run CEinfo.pl and read ldif-script.sh
chmod -R 777 /tmp/run/arc/bdii
chmod -R 777 /var/run/arc/infosys
chmod -R 777 /tmp/tmp/arc/bdii

# please see ldif-print-test-arc.conf and pass it to CEinfo.pl
# ./CEinfo.pl --config test/ldif-print-test-arc.conf
