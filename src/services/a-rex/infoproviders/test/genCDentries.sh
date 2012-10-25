#!/bin/bash

#######################
#
# Creates jobs in controldir
#
########################

##

if [ "$1" = 'help' ]
    then
    echo 'usage: ./genCDentries.sh jobID numberofjobentries jobcontroldir';
    exit 0;
fi

JOBID=$1;
NUMENTRIES=$2;
CONTROLDIR=$3;

if [ -z "$3" ] 
	then
	$CONTROLDIR="."
fi

for (( i=1; i <= ${NUMENTRIES} ; i++ ))
do
    rid=`< /dev/urandom tr -dc A-Za-z0-9 | head -c 54`
    for ext in description diag errors grami input input_status local output output_status rte proxy statistics
    do
       cp $CONTROLDIR/job.$JOBID.$ext $CONTROLDIR/job.$rid.$ext
    done
	cp $CONTROLDIR/finished/job.$JOBID.status $CONTROLDIR/finished/job.$rid.status
done
