#!/usr/bin/perl

use strict;
use InfoproviderTestSuite;

my $suite = new InfoproviderTestSuite('slurmpy');

# TODO: Multiple queues, multiple users, multiple freecpus (if possible by slurm), multiple nodes, multiple comments?, multiple nodes.
$suite->test("basic", sub {
  my @progs = qw(sinfo scontrol squeue);
  my $simulator_output = <<ENDSIMULATOROUTPUT;
# Output from SLURM version 2.3.2
args="sinfo -a -h -o PartitionName=%P TotalCPUs=%C TotalNodes=%D MaxTime=%l"
output="PartitionName=queue1 TotalCPUs=0/1/0/1 TotalNodes=1 MaxTime=infinite"

# Output from SLURM version 2.3.2
args="sinfo -a -h -o cpuinfo=%C"
output="cpuinfo=0/1/0/1"
args="sinfo -a -h -o %C"
output="0/1/0/1"

# Output from SLURM version 2.3.2
args="scontrol show node --oneliner"
output="NodeName=test-machine Arch=i686 CoresPerSocket=1 CPUAlloc=0 CPUErr=0 CPUTot=1 Features=(null) Gres=(null) NodeAddr=test-machine NodeHostName=test-machine OS=Linux RealMemory=1 Sockets=1 State=IDLE ThreadsPerCore=1 TmpDisk=0 Weight=1 BootTime=2014-05-20T15:13:45 SlurmdStartTime=2014-05-20T15:13:50 Reason=(null)"

# Output from SLURM version 2.3.2
args="squeue -a -h -t all -o JobId=%i TimeUsed=%M Partition=%P JobState=%T ReqNodes=%D ReqCPUs=%C TimeLimit=%l Name=%j NodeList=%N"
output=<<<ENDOUTPUT
JobId=2 TimeUsed=0:00 Partition=queue1 JobState=PENDING ReqNodes=1 ReqCPUs=1 TimeLimit=6:00 Name=test job NodeList=
JobId=1 TimeUsed=1:23 Partition=queue1 JobState=RUNNING ReqNodes=1 ReqCPUs=1 TimeLimit=6:00 Name=test job NodeList=test-machine
ENDOUTPUT

# Output from SLURM version 2.3.2
args="scontrol show config"
output=<<<ENDOUTPUT
Configuration data as of 2014-05-21T18:52:38
AccountingStorageBackupHost = (null)
AccountingStorageEnforce = none
AccountingStorageHost   = localhost
AccountingStorageLoc    = /var/log/slurm_jobacct.log
AccountingStoragePort   = 0
AccountingStorageType   = accounting_storage/none
AccountingStorageUser   = root
AccountingStoreJobComment = YES
AuthType                = auth/munge
BackupAddr              = (null)
BackupController        = (null)
BatchStartTimeout       = 10 sec
BOOT_TIME               = 2014-05-20T15:13:50
CacheGroups             = 0
CheckpointType          = checkpoint/none
ClusterName             = cluster
CompleteWait            = 0 sec
ControlAddr             = test-machine
ControlMachine          = test-machine
CryptoType              = crypto/munge
DebugFlags              = (null)
DefMemPerNode           = UNLIMITED
DisableRootJobs         = NO
EnforcePartLimits       = NO
Epilog                  = (null)
EpilogMsgTime           = 2000 usec
EpilogSlurmctld         = (null)
FastSchedule            = 1
FirstJobId              = 1
GetEnvTimeout           = 2 sec
GresTypes               = (null)
GroupUpdateForce        = 0
GroupUpdateTime         = 600 sec
HASH_VAL                = Match
HealthCheckInterval     = 0 sec
HealthCheckProgram      = (null)
InactiveLimit           = 0 sec
JobAcctGatherFrequency  = 30 sec
JobAcctGatherType       = jobacct_gather/none
JobCheckpointDir        = /var/lib/slurm-llnl/checkpoint
JobCompHost             = localhost
JobCompLoc              = /var/log/slurm_jobcomp.log
JobCompPort             = 0
JobCompType             = jobcomp/none
JobCompUser             = root
JobCredentialPrivateKey = (null)
JobCredentialPublicCertificate = (null)
JobFileAppend           = 0
JobRequeue              = 1
JobSubmitPlugins        = (null)
KillOnBadExit           = 0
KillWait                = 30 sec
Licenses                = (null)
MailProg                = /usr/bin/mail
MaxJobCount             = 10000
MaxJobId                = 4294901760
MaxMemPerNode           = UNLIMITED
MaxStepCount            = 40000
MaxTasksPerNode         = 128
MessageTimeout          = 10 sec
MinJobAge               = 300 sec
MpiDefault              = none
MpiParams               = (null)
NEXT_JOB_ID             = 19
OverTimeLimit           = 0 min
PluginDir               = /usr/lib/slurm
PlugStackConfig         = /etc/slurm-llnl/plugstack.conf
PreemptMode             = OFF
PreemptType             = preempt/none
PriorityType            = priority/basic
PrivateData             = none
ProctrackType           = proctrack/pgid
Prolog                  = (null)
PrologSlurmctld         = (null)
PropagatePrioProcess    = 0
PropagateResourceLimits = ALL
PropagateResourceLimitsExcept = (null)
ResumeProgram           = (null)
ResumeRate              = 300 nodes/min
ResumeTimeout           = 60 sec
ResvOverRun             = 0 min
ReturnToService         = 1
SallocDefaultCommand    = (null)
SchedulerParameters     = (null)
SchedulerPort           = 7321
SchedulerRootFilter     = 1
SchedulerTimeSlice      = 30 sec
SchedulerType           = sched/backfill
SelectType              = select/linear
SlurmUser               = slurm(64030)
SlurmctldDebug          = 3
SlurmctldLogFile        = /var/log/slurm-llnl/slurmctld.log
SlurmSchedLogFile       = (null)
SlurmctldPort           = 6817
SlurmctldTimeout        = 300 sec
SlurmdDebug             = 3
SlurmdLogFile           = /var/log/slurm-llnl/slurmd.log
SlurmdPidFile           = /var/run/slurm-llnl/slurmd.pid
SlurmdPort              = 6818
SlurmdSpoolDir          = /var/lib/slurm-llnl/slurmd
SlurmdTimeout           = 300 sec
SlurmdUser              = root(0)
SlurmSchedLogLevel      = 0
SlurmctldPidFile        = /var/run/slurm-llnl/slurmctld.pid
SLURM_CONF              = /etc/slurm-llnl/slurm.conf
SLURM_VERSION           = 2.3.2
SrunEpilog              = (null)
SrunProlog              = (null)
StateSaveLocation       = /var/lib/slurm-llnl/slurmctld
SuspendExcNodes         = (null)
SuspendExcParts         = (null)
SuspendProgram          = (null)
SuspendRate             = 60 nodes/min
SuspendTime             = NONE
SuspendTimeout          = 30 sec
SwitchType              = switch/none
TaskEpilog              = (null)
TaskPlugin              = task/none
TaskPluginParam         = (null type)
TaskProlog              = (null)
TmpFS                   = /tmp
TopologyPlugin          = topology/none
TrackWCKey              = 0
TreeWidth               = 50
UsePam                  = 0
UnkillableStepProgram   = (null)
UnkillableStepTimeout   = 60 sec
VSizeFactor             = 0 percent
WaitTime                = 0 sec

Slurmctld(primary/backup) at test-machine/(NULL) are UP/DOWN
ENDOUTPUT
ENDSIMULATOROUTPUT
  
  my $cfg = {  slurm_bin_path => "<TESTDIR>/bin",
               queues => { queue1 => { users => ['user1'] } },
               jobs => ['1', '2'],
               loglevel => '5'
            };
            
  my $lrms_info = $suite->collect(\@progs, $simulator_output, $cfg);

  is(ref $lrms_info, 'HASH', 'result type');

  is(ref $lrms_info->{cluster}, 'HASH', 'has cluster');
  is(lc($lrms_info->{cluster}{lrms_type}), 'slurm', 'lrms_type');
  is($lrms_info->{cluster}{lrms_version}, '2.3.2', 'lrms_version');
  is($lrms_info->{cluster}{schedpolicy}, undef, 'schedpolicy');
  is($lrms_info->{cluster}{totalcpus}, 1, 'totalcpus');
  is($lrms_info->{cluster}{queuedcpus}, 1, 'queuedcpus');
  is($lrms_info->{cluster}{usedcpus}, 0, 'usedcpus');
  is($lrms_info->{cluster}{queuedjobs}, 1, 'queuedjobs');
  is($lrms_info->{cluster}{runningjobs}, 1, 'runningjobs');
  is($lrms_info->{cluster}{cpudistribution}, '1cpu:1', 'cpudistribution');

  is(ref $lrms_info->{queues}, 'HASH', 'has queues');
  is(ref $lrms_info->{queues}{queue1}, 'HASH', 'has queue1');
  is($lrms_info->{queues}{queue1}{status}, 10000, 'queue1->status');
  is($lrms_info->{queues}{queue1}{maxrunning}, 10000, 'queue1->maxrunning');
  is($lrms_info->{queues}{queue1}{maxqueuable}, 10000, 'queue1->maxqueuable');
  is($lrms_info->{queues}{queue1}{maxuserrun}, 10000, 'queue1->maxuserrun');
  is($lrms_info->{queues}{queue1}{maxcputime}, 2**31-1, 'queue1->maxcputime');
  is($lrms_info->{queues}{queue1}{maxtotalcputime}, undef, 'queue1->maxtotalcputime');
  is($lrms_info->{queues}{queue1}{mincputime}, 0, 'queue1->mincputime');
  is($lrms_info->{queues}{queue1}{defaultcput}, 2**31-1, 'queue1->defaultcput');
  is($lrms_info->{queues}{queue1}{maxwalltime}, 2**31-1, 'queue1->maxwalltime');
  is($lrms_info->{queues}{queue1}{minwalltime}, 0, 'queue1->minwalltime');
  is($lrms_info->{queues}{queue1}{defaultwallt}, 2**31-1, 'queue1->defaultwallt');
  is($lrms_info->{queues}{queue1}{running}, 1, 'queue1->running');
  is($lrms_info->{queues}{queue1}{queued}, 1, 'queue1->queued');
  is($lrms_info->{queues}{queue1}{suspended}, undef, 'queue1->suspended');
  is($lrms_info->{queues}{queue1}{total}, undef, 'queue1->total');
  is($lrms_info->{queues}{queue1}{totalcpus}, 1, 'queue1->totalcpus');
  is($lrms_info->{queues}{queue1}{preemption}, undef, 'queue1->preemption');
  is($lrms_info->{queues}{queue1}{'acl_users'}, undef, 'queue1->acl_users');
  
  is(ref $lrms_info->{queues}{queue1}{users}, 'HASH', 'has users');
  is(ref $lrms_info->{queues}{queue1}{users}{user1}, 'HASH', 'has user1');
  is_deeply($lrms_info->{queues}{queue1}{users}{user1}{freecpus}, { '1' => 0 }, 'user1->freecpus');
  is($lrms_info->{queues}{queue1}{users}{user1}{queuelength}, 0, 'user1->queuelength');

  is(ref $lrms_info->{jobs}, 'HASH', 'has jobs');

  is(ref $lrms_info->{jobs}{'1'}, 'HASH', 'has job id = 1');
  is($lrms_info->{jobs}{'1'}{status}, 'R', 'job1->status');
  is($lrms_info->{jobs}{'1'}{cpus}, 1, 'job1->cpus');
  is($lrms_info->{jobs}{'1'}{rank}, 0, 'job1->rank');
  is($lrms_info->{jobs}{'1'}{mem}, 1, 'job1->mem');
  is($lrms_info->{jobs}{'1'}{walltime}, 83, 'job1->walltime');
  is($lrms_info->{jobs}{'1'}{cputime}, 83, 'job1->cputime');
  is($lrms_info->{jobs}{'1'}{reqwalltime}, 360, 'job1->reqwalltime');
  is($lrms_info->{jobs}{'1'}{reqcputime}, 360, 'job1->reqcputime');
  is(ref $lrms_info->{jobs}{'1'}{nodes}, 'ARRAY', 'job1->nodes is ARRAY');
  is(scalar @{$lrms_info->{jobs}{'1'}{nodes}}, 1, 'job1->nodes size');
  is($lrms_info->{jobs}{'1'}{nodes}[0], 'test-machine', 'job1->nodes->0');
  is(ref $lrms_info->{jobs}{'1'}{comment}, 'ARRAY', 'job1->comment');
  is(scalar @{$lrms_info->{jobs}{'1'}{comment}}, 1, 'job1->comment');
  is($lrms_info->{jobs}{'1'}{comment}[0], 'test job', 'job1->comment->0');

  is(ref $lrms_info->{jobs}{'2'}, 'HASH', 'has job id = 2');
  is($lrms_info->{jobs}{'2'}{status}, 'Q', 'job2->status');
  is($lrms_info->{jobs}{'2'}{cpus}, 1, 'job2->cpus');
  is($lrms_info->{jobs}{'2'}{rank}, 0, 'job2->rank');
  is($lrms_info->{jobs}{'2'}{mem}, undef, 'job2->mem');
  is($lrms_info->{jobs}{'2'}{walltime}, 0, 'job2->walltime');
  is($lrms_info->{jobs}{'2'}{cputime}, 0, 'job2->cputime');
  is($lrms_info->{jobs}{'2'}{reqwalltime}, 360, 'job2->reqwalltime');
  is($lrms_info->{jobs}{'2'}{reqcputime}, 360, 'job2->reqcputime');
  is(ref $lrms_info->{jobs}{'2'}{nodes}, 'ARRAY', 'job2->nodes is ARRAY');
  is(scalar @{$lrms_info->{jobs}{'2'}{nodes}}, 0, 'job2->nodes size');
  is(ref $lrms_info->{jobs}{'2'}{comment}, 'ARRAY', 'job2->comment');
  is(scalar @{$lrms_info->{jobs}{'2'}{comment}}, 1, 'job2->comment');
  is($lrms_info->{jobs}{'2'}{comment}[0], 'test job', 'job2->comment->0');

  is(ref $lrms_info->{nodes}, 'HASH', 'has nodes');
  is(ref $lrms_info->{nodes}{"test-machine"}, 'HASH', 'has test-machine');
  is($lrms_info->{nodes}{"test-machine"}{isavailable}, 1, 'test-machine->isavailable');
  is($lrms_info->{nodes}{"test-machine"}{isfree}, 1, 'test-machine->isfree');
  is($lrms_info->{nodes}{"test-machine"}{tags}, undef, 'test-machine->tags');
  is($lrms_info->{nodes}{"test-machine"}{vmem}, undef, 'test-machine->vmem');
  is($lrms_info->{nodes}{"test-machine"}{pmem}, 1, 'test-machine->pmem');
  is($lrms_info->{nodes}{"test-machine"}{slots}, 1, 'test-machine->slots');
  is($lrms_info->{nodes}{"test-machine"}{pcpus}, 1, 'test-machine->pcpus');
  is($lrms_info->{nodes}{"test-machine"}{sysname}, 'Linux', 'test-machine->sysname');
  is($lrms_info->{nodes}{"test-machine"}{release}, undef, 'test-machine->release');
  is($lrms_info->{nodes}{"test-machine"}{machine}, 'i686', 'test-machine->machine');
});

$suite->testing_done();
