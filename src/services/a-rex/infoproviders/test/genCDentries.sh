#!/bin/bash

#######################
#
# Creates jobs in controldir
#
########################

##

if [ "$1" = 'help' ]
    then
    echo 'usage: ./genCDentries.sh jobID numberofjobentries';
fi

JOBID=$1;
NUMENTRIES=$2;

for (( i=1; i <= ${NUMENTRIES} ; i++ ))
do
    rid=`< /dev/urandom tr -dc A-Za-z0-9 | head -c 54`
    for ext in description diag errors grami input input_status local output output_status rte proxy statistics
    do
       cp job.$JOBID.$ext job.$rid.$ext
    done
	cp finished/job.$JOBID.status finished/job.$rid.status
done
