#!/bin/bash -x

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



# create symlink to python parser
echo "Creating symlink to python parser..."
ln -fs ../../../utils/configparser/arcconfig-parser arcconfig-parser
echo ""

echo "please see test-arc.conf and pass it to CEinfo.pl"
echo "AREX call: ./CEinfo.pl --splitjobs --config test/test-arc.conf"
echo "Simple execution that includes jobs in XML and LDIF: sudo ./CEinfo.pl --splitjobs --config test/test-arc.conf" 

