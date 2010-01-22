#!/bin/bash
  
# PBS test suite pbs-1
#   Torque version  2.3.7
#   2 nodes (1 down) of 2 cpus each
#   1 queue, no jobs
#
# Defines behaviour for:
#    qmgr -c list server
#    pbsnodes -a
#    qstat -f
#    qstat -f -Q queue1
#    qstat -Q
#    qstat -n


cmd="$(basename $0) $*"

case $cmd in

#######################################
  'qmgr -c list server')
    cat <<ENDL
Server head.domain.com
	server_state = Active
	scheduling = True
	total_jobs = 0
	state_count = Transit:0 Queued:0 Held:0 Waiting:0 Running:0 Exiting:0 
	acl_host_enable = True
	acl_hosts = *.domain.com
	log_events = 511
	mail_from = adm
	resources_assigned.ncpus = 0
	resources_assigned.nodect = 0
	scheduler_iteration = 600
	node_check_rate = 150
	tcp_timeout = 6
	pbs_version = 2.3.7
	next_job_number = 496
	net_counter = 10 3 1

ENDL
    exit 0
    ;;

#######################################
  "pbsnodes -a")
    cat <<ENDF
node1
     state = free
     np = 2
     properties = queue1,x86_64
     ntype = cluster
     status = opsys=linux,uname=Linux node1.domain.com 2.6.18-164.6.1.el5 #1 SMP Tue Oct 27 11:28:30 EDT 2009 x86_64,sessions=1720 7063 7781 21430,nsessions=4,nusers=2,idletime=72301,totmem=5220132kb,availmem=4851656kb,physmem=1025836kb,ncpus=1,loadave=0.36,netload=382898619738,state=free,jobs=,varattr=,rectime=1263989477

ENDF
    exit 0
    ;;

#######################################
  'qstat -f')
    cat <<ENDF
ENDF
    exit 0
    ;;

#######################################
  'qstat -f -Q queue1')
    cat <<ENDF
Queue: queue1
    queue_type = Execution
    total_jobs = 0
    state_count = Transit:0 Queued:0 Held:0 Waiting:0 Running:0 Exiting:0 
    max_running = 10
    mtime = 1263485241
    resources_assigned.ncpus = 0
    resources_assigned.nodect = 0
    enabled = True
    started = True

ENDF
    exit 0
    ;;

#######################################
  'qstat -Q')
    cat <<ENDF
Queue              Max   Tot   Ena   Str   Que   Run   Hld   Wat   Trn   Ext T         
----------------   ---   ---   ---   ---   ---   ---   ---   ---   ---   --- -         
queue1              10     0   yes   yes     0     0     0     0     0     0 E         
ENDF
    exit 0
    ;;

#######################################
  'qstat -n')
    cat <<ENDF
ENDF
    exit 0
    ;;

#######################################
  *)
    echo $0: No match for: $* 2>&1
    exit 1
    ;;
esac
