#!/bin/bash

# Script to create dev environment for perl infoproviders
# author: florido.paganelli@hep.lu.se

function showhelp {
HELPSTR=$(cat <<EOF
   This script initializes the perl dev environment for infoproviders.
   Creates folders and symlinks to test scripts without the make step.
   Options:
      -h show this help
      -l create fake bdii folders in /tmp to test ldap output
EOF
)

echo "   Usage: $0 [options]"
echo ""
echo "$HELPSTR" 
echo ""

}

# initialize ldap

function initldap {

#if [ $UID != 0 ] 
#   then
#       echo "Sorry, this script must be currently run as root to create ldap folders"
#       exit 1
#fi

#echo "This script must NOT be run on a running cluster, as it might compromise"
#echo "infosys behaviour. Use with care! Only for offline development."

#read -p "Continue? y/n"
#if [ "$REPLY" != 'y' ] 
#   then
#   echo "Please execute script again and enter \"y\" if you really want to continue."
#   exit 0
#fi

# this script is intended to setup the needed
# for testing ldif printing.
# will assume /tmp/ as the base dir for BDII stuff.

# create fake bdii dirs in 
echo "creating fake bdii dirs in /tmp..."
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
echo "Chmodding dirs 777 for all users to access..."
chmod -R 777 /tmp/run/arc/bdii
chmod -R 777 /tmp/run/arc/infosys
chmod -R 777 /tmp/tmp/arc/bdii

echo ""
}

while getopts ":lh" opt; do
  case $opt in
    l)
      echo "ldap dev enabled"
      initldap
      ;;
    h)
      showhelp
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      showhelp
      exit 1
      ;;
  esac
done

# check that we are in the correct dir
if ! [ -e CEinfo.pl.in ]; then
   echo "Wrong directory. Run this script from the infoproviders source"
   echo "folder with"
   echo "source test/initdevenv.sh <options>"
   exit 1
fi

# create symlink to python parser
echo "Creating symlink to python parser..."
mkdir -p /tmp/libexec/arc/
SRCABSOLUTEPATH=$(readlink -f ../../../)
ln -fs $SRCABSOLUTEPATH/utils/python/arcconfig-parser /tmp/libexec/arc/arcconfig-parser
echo "Location: $SRCABSOLUTEPATH/utils/python/arcconfig-parser"

echo "extending ARC_LOCATION and PYTHONPATH"
export ARC_LOCATION='/tmp'
export PYTHONPATH=$SRCABSOLUTEPATH/utils/python/:$PYTHONPATH
echo "Location: $PYTHONPATH"
echo ""

# missing file in module tree, I hope this is a temporary fix...
echo "adding __init__.py to $SRCABSOLUTEPATH/utils/python/arc/utils/ ..."
touch $SRCABSOLUTEPATH/utils/python/arc/utils/__init__.py

# create defaults file
echo "Creating defaults file $ARC_LOCATION/parser.defaults"
$SRCABSOLUTEPATH/utils/python/arcconfig-reference --extract-defaults -r  $SRCABSOLUTEPATH/doc/arc.conf.reference > $ARC_LOCATION/parser.defaults

echo "To generate runtime conf, run:"
echo "../../../utils/python/arcconfig-parser --save -d /tmp/parser.defaults -c test/test-arc.conf -r /tmp/arc.runtime.conf"
echo

echo "modify test/test-arc.conf and pass it to CEinfo.pl"
echo "AREX call: ./CEinfo.pl --splitjobs --config /tmp/arc.runtime.conf"
echo "Simple execution that includes jobs in XML and LDIF: sudo ./CEinfo.pl --splitjobs --config /tmp/arc.runtime.conf" 

