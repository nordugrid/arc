#!/bin/bash

# this script is intended to setup the needed
# for testing ldif printing.
# at the moment there is some problem with arc.conf parsing on
# infoproviders regarding bdii, so it doesn't really work as
# it should, you need to be root to execute.
# DO NOT RUN ON PRODUCTION ENVIRONMENT will overwrite bdii stuff

# create fake bdii dirs
mkdir -p /var/run/arc/{bdii,infosys}

# create fake bdii pidfile
touch /var/run/arc/bdii/bdii-update.pid

# chmod for normal user to run CEinfo.pl and read ldif-script.sh
chmod -R 777 /var/run/arc/bdii
chmod -R 777 /var/run/arc/infosys

# please see ldif-print-test-arc.conf and pass it to CEinfo.pl
# ./CEinfo.pl --config test/ldif-print-test-arc.conf
