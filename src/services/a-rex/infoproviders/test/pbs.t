#!/usr/bin/perl

use strict;
use InfoproviderTestSuite;

my $suite = new InfoproviderTestSuite('pbs');

$suite->test("basic", sub {
  my @progs = qw(qmgr qstat pbsnodes);
  my $simulator_output = <<ENDSIMULATOROUTPUT;
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
#    qstat -Q -f queue1
#    qstat -Q
#    qstat -n

#######################################
args="qmgr -c list server"
output=<<<ENDL
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

#######################################
args="pbsnodes -a"
output=<<<ENDF
node1
     state = free
     np = 2
     properties = queue1,x86_64
     ntype = cluster
     status = opsys=linux,uname=Linux node1.domain.com 2.6.18-164.6.1.el5 #1 SMP Tue Oct 27 11:28:30 EDT 2009 x86_64,sessions=1720 7063 7781 21430,nsessions=4,nusers=2,idletime=72301,totmem=5220132kb,availmem=4851656kb,physmem=1025836kb,ncpus=1,loadave=0.36,netload=382898619738,state=free,jobs=,varattr=,rectime=1263989477

ENDF

#######################################
args="qstat -f"
output=""

#######################################
args="qstat -f -Q queue1"
output=<<<ENDF
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

#######################################
args="qstat -Q"
output=<<<ENDF
Queue              Max   Tot   Ena   Str   Que   Run   Hld   Wat   Trn   Ext T         
----------------   ---   ---   ---   ---   ---   ---   ---   ---   ---   --- -         
queue1              10     0   yes   yes     0     0     0     0     0     0 E         
ENDF

#######################################
args="qstat -n"
output=""

#######################################
args="qstat -Q -f queue1"
output=<<<ENDF
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
ENDSIMULATOROUTPUT

  my $cfg = {
               pbs_bin_path => "<TESTDIR>/bin",
               pbs_log_path => "<TESTDIR>/bin",
               dedicated_node_string => '', # NB: it's set in order to avoid perl warnings 
               queues => {},
               jobs => []
            };
  $cfg->{queues}{queue1} = {users => ['user1']};
  
  my $lrms_info = $suite->collect(\@progs, $simulator_output, $cfg);
  
  is(ref $lrms_info, 'HASH', 'result type');
  is(ref $lrms_info->{cluster}, 'HASH', 'has cluster');
  is($lrms_info->{cluster}{lrms_type}, 'torque', 'lrms_type');
  is($lrms_info->{cluster}{lrms_version}, '2.3.7', 'lrms_version');
  is($lrms_info->{cluster}{runningjobs}, 0, 'runningjobs');
  is($lrms_info->{cluster}{totalcpus}, 2, 'totalcpus');
  is($lrms_info->{cluster}{queuedcpus}, 0, 'queuedcpus');
  is($lrms_info->{cluster}{queuedjobs}, 0, 'queuedjobs');
  is($lrms_info->{cluster}{usedcpus}, 0, 'usedcpus');
  is($lrms_info->{cluster}{cpudistribution}, '2cpu:1', 'cpudistribution');
  
  is(ref $lrms_info->{queues}, 'HASH', 'has queues');
  is(ref $lrms_info->{queues}{queue1}, 'HASH', 'has queue1');
  ok($lrms_info->{queues}{queue1}{status} >= 0, 'queue1->status');
  is($lrms_info->{queues}{queue1}{totalcpus}, 2, 'queue1->totalcpus');
  is($lrms_info->{queues}{queue1}{queued}, 0, 'queue1->queued');
  is($lrms_info->{queues}{queue1}{running}, 0, 'queue1->running');
  is($lrms_info->{queues}{queue1}{maxrunning}, 10, 'queue1->maxrunning');
  
  is(ref $lrms_info->{queues}{queue1}{users}, 'HASH', 'has users');
  is(ref $lrms_info->{queues}{queue1}{users}{user1}, 'HASH', 'has user1');
  is($lrms_info->{queues}{queue1}{users}{user1}{queuelength}, 0, 'user1->queuelenght');
  is_deeply($lrms_info->{queues}{queue1}{users}{user1}{freecpus}, { '2' => 0 }, 'user1->freecpus');
  
  is(ref $lrms_info->{nodes}, 'HASH', 'has nodes');
  is(ref $lrms_info->{nodes}{node1}, 'HASH', 'has node1');
  is($lrms_info->{nodes}{node1}{isavailable}, 1, 'node1->isavailable');
  is($lrms_info->{nodes}{node1}{slots}, 2, 'node1->slots');
});

$suite->testing_done();
