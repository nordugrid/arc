#!/bin/sh -f
# arguments: job_status job_id control_directory local_mail job_name failure_reason list of addresses

basedir=`dirname $0`
basedir=`cd $basedir; pwd`

if [ -z "$ARC_LOCATION" ] ; then
  echo "ARC_LOCATION is not defined" 1>&2
  exit 1
fi

if [ $# -lt 7 ] ; then
  echo "Not enough arguments" 1>&2
  exit 1
fi

# arguments
status=$1
shift
job_id=$1
shift
control_dir=$1
shift
local_mail=$1
shift
job_name=$1
shift
failure_reason=$1
shift
if [ -z "$local_mail" ] ; then
  echo "Empty local mail address" 1>&2
  exit 1
fi

#host_name=`hostname -f`
cur_time=`date -R`
cluster_name=`hostname --fqdn`

while true ; do
  if [ $# -lt 1 ] ; then break ; fi
  mail_addr=$1
  if [ -z "$mail_addr" ] ; then break; fi
  (
#  job_name=`cat $control_dir/job.$job_id.local 2>/dev/null | \
#            sed --quiet 's/^jobname=\(.*\)/\1/;t print;s/.*//;t;:print;p'`  
#  if [ -z "$job_name" ] ; then
#    job_name='<undefined name>'
#  fi
  echo "From: $local_mail"
  echo "To: $mail_addr"
  if [ -z "$job_name" ] ; then
    echo "Subject: Message from job $job_id"
  else
    echo "Subject: Message from job $job_name ($job_id)"
  fi
  echo "Date: $cur_time"
  echo
  if [ ! -z "$job_name" ] ; then
    job_name="\"$job_name\" "
  fi
  job_name="${job_name}(${job_id})"
  if [ ! -z "$cluster_name" ] ; then
    job_name="${job_name} at ${cluster_name}"
  fi
  if [ ! -z "$failure_reason" ] ; then
    echo "Job $job_name state is $status. Job FAILED with reason:"
    echo "$failure_reason"
    if [ "$status" = FINISHED ] ; then
      if [ -r "$control_dir/job.$job_id.diag" ] ; then
        grep -i '^WallTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^KernelTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^UserTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^MaxResidentMemory' "$control_dir/job.$job_id.diag" 2>/dev/null
      fi
      # Oxana requested more information. Race conditions are possible here
      if [ -r "$control_dir/job.$job_id.local" ] ; then
        grep -i '^queue' "$control_dir/job.$job_id.local" 2>/dev/null
        grep -i '^starttime' "$control_dir/job.$job_id.local" 2>/dev/null
        grep -i '^cleanuptime' "$control_dir/job.$job_id.local" 2>/dev/null
      fi
    fi
    if [ -f "$control_dir/job.$job_id.errors" ] ; then
      echo
      echo 'Following is the log of job processing:'
      echo '-------------------------------------------------'
      cat "$control_dir/job.$job_id.errors" 2>/dev/null
      echo '-------------------------------------------------'
      echo
    fi
  else
    echo "Job $job_name current state is $status."
    if [ "$status" = FINISHED ] ; then
      if [ -r "$control_dir/job.$job_id.diag" ] ; then
        grep -i '^WallTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^KernelTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^UserTime' "$control_dir/job.$job_id.diag" 2>/dev/null
        grep -i '^MaxResidentMemory' "$control_dir/job.$job_id.diag" 2>/dev/null
      fi
      if [ -r "$control_dir/job.$job_id.local" ] ; then
        grep -i '^queue' "$control_dir/job.$job_id.local" 2>/dev/null
        grep -i '^starttime' "$control_dir/job.$job_id.local" 2>/dev/null
        grep -i '^cleanuptime' "$control_dir/job.$job_id.local" 2>/dev/null
      fi
    fi
  fi
  ) | \
  $basedir/smtp-send "$local_mail" "$mail_addr"
  shift
done
