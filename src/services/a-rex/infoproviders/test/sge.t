#!/usr/bin/perl

# TODO: This test is broken!

use strict;
use InfoproviderTestSuite;

my $suite = new InfoproviderTestSuite('sge');

$suite->test('basic', sub {
  my @progs = qw(qsub qstat qconf qhost);
  my $simulator_output = <<'ENDSIMULATOROUTPUT';
# SGE test suite sge-1
#   SGE version  6.2
#   1 nodes 1 cpus
#   1 queue, no jobs
#
# Defines behaviour for:
# qstat -help
# qstat -u '*' -f (would neeed -F for older SGE)
# qconf -sep
# qconf -sconf global
# qconf -sql
# qconf -sq all.q
# qstat -j 

## Also these for the old module:
# qstat -g c
# qstat -u * -s rs
# qstat -u '*' -s p


args="qstat -help"
output=<<<ENDOUTPUT
SGE 6.2
usage: qstat [options]
        [-ext]                            view additional attributes
        [-explain a|c|A|E]                show reason for c(onfiguration ambiguous), a(larm), suspend A(larm), E(rror) state
        [-f]                              full output
        [-F [resource_attributes]]        full output and show (selected) resources of queue(s)
        [-g {c}]                          display cluster queue summary
        [-g {d}]                          display all job-array tasks (do not group)
        [-g {t}]                          display all parallel job tasks (do not group)
        [-help]                           print this help
        [-j job_identifier_list ]         show scheduler job information
        [-l resource_list]                request the given resources
        [-ne]                             hide empty queues
        [-pe pe_list]                     select only queues with one of these parallel environments
        [-q wc_queue_list]                print information on given queue
        [-qs {a|c|d|o|s|u|A|C|D|E|S}]     selects queues, which are in the given state(s)
        [-r]                              show requested resources of job(s)
        [-s {p|r|s|z|hu|ho|hs|hd|hj|ha|h|a}] show pending, running, suspended, zombie jobs,
                                          jobs with a user/operator/system/array-dependency hold, 
                                          jobs with a start time in future or any combination only.
                                          h is an abbreviation for huhohshdhjha
                                          a is an abbreviation for prsh
        [-t]                              show task information (implicitly -g t)
        [-u user_list]                    view only jobs of this user
        [-U user_list]                    select only queues where these users have access
        [-urg]                            display job urgency information
        [-pri]                            display job priority information
        [-xml]                            display the information in XML-Format

pe_list                  pe[,pe,...]
job_identifier_list      [job_id|job_name|pattern]{, [job_id|job_name|pattern]}
resource_list            resource[=value][,resource[=value],...]
user_list                user|@group[,user|@group],...]
resource_attributes      resource,resource,...
wc_cqueue                wildcard expression matching a cluster queue
wc_host                  wildcard expression matching a host
wc_hostgroup             wildcard expression matching a hostgroup
wc_qinstance             wc_cqueue@wc_host
wc_qdomain               wc_cqueue@wc_hostgroup
wc_queue                 wc_cqueue|wc_qdomain|wc_qinstance
wc_queue_list            wc_queue[,wc_queue,...]
ENDOUTPUT

args="qstat -u \'*\' -f"
output=<<<ENDOUTPUT
queuename                      qtype resv/used/tot. load_avg arch          states
---------------------------------------------------------------------------------
all.q@node1.site.org           BIP   0/0/1          0.17     lx24-amd64    
ENDOUTPUT

args="qstat -u * -f"
output=<<<ENDOUTPUT
queuename                      qtype resv/used/tot. load_avg arch          states
---------------------------------------------------------------------------------
all.q@node1.site.org           BIP   0/0/1          0.17     lx24-amd64    
ENDOUTPUT

args="qconf -sep"
output=<<<ENDOUTPUT
HOST                      PROCESSOR        ARCH
===============================================
node1.site.org                    1  lx24-amd64
===============================================
SUM                               1
ENDOUTPUT

args="qconf -sconf global"
output=<<<ENDOUTPUT
#global:
execd_spool_dir              /opt/n1ge6/x/spool
mailer                       /bin/mail
xterm                        /usr/bin/X11/xterm
load_sensor                  none
prolog                       none
epilog                       none
shell_start_mode             posix_compliant
login_shells                 sh,ksh,csh,tcsh
min_uid                      0
min_gid                      0
user_lists                   none
xuser_lists                  none
projects                     none
xprojects                    none
enforce_project              false
enforce_user                 auto
load_report_time             00:00:40
max_unheard                  00:05:00
reschedule_unknown           00:00:00
loglevel                     log_warning
administrator_mail           none
set_token_cmd                none
pag_cmd                      none
token_extend_time            none
shepherd_cmd                 none
qmaster_params               none
execd_params                 none
reporting_params             accounting=true reporting=false \
                             flush_time=00:00:15 joblog=false sharelog=00:00:00
finished_jobs                100
gid_range                    20000-20100
qlogin_command               builtin
qlogin_daemon                builtin
rlogin_command               builtin
rlogin_daemon                builtin
rsh_command                  builtin
rsh_daemon                   builtin
max_aj_instances             2000
max_aj_tasks                 75000
max_u_jobs                   0
max_jobs                     0
max_advance_reservations     0
auto_user_oticket            0
auto_user_fshare             0
auto_user_default_project    none
auto_user_delete_time        86400
delegated_file_staging       false
reprioritize                 0

ENDOUTPUT

args="qconf -sql"
output="all.q"

args="qconf -sq all.q"
output=<<<ENDOUTPUT
qname                 all.q
hostlist              @allhosts
seq_no                0
load_thresholds       np_load_avg=1.75
suspend_thresholds    NONE
nsuspend              1
suspend_interval      00:05:00
priority              0
min_cpu_interval      00:05:00
processors            UNDEFINED
qtype                 BATCH INTERACTIVE
ckpt_list             NONE
pe_list               make
rerun                 FALSE
slots                 1,[node1.site.org=1]
tmpdir                /tmp
shell                 /bin/sh
prolog                NONE
epilog                NONE
shell_start_mode      posix_compliant
starter_method        NONE
suspend_method        NONE
resume_method         NONE
terminate_method      NONE
notify                00:00:60
owner_list            NONE
user_lists            NONE
xuser_lists           NONE
subordinate_list      NONE
complex_values        NONE
projects              NONE
xprojects             NONE
calendar              NONE
initial_state         default
s_rt                  INFINITY
h_rt                  INFINITY
s_cpu                 INFINITY
h_cpu                 INFINITY
s_fsize               INFINITY
h_fsize               INFINITY
s_data                INFINITY
h_data                INFINITY
s_stack               INFINITY
h_stack               INFINITY
s_core                INFINITY
h_core                INFINITY
s_rss                 INFINITY
h_rss                 INFINITY
s_vmem                INFINITY
h_vmem                INFINITY
ENDOUTPUT

args="qstat -j"
output=<<<ENDOUTPUT
scheduling info:            (Collecting of scheduler job information is turned off)

ENDOUTPUT

args="qstat -g c"
output=<<<ENDOUTPUT
CLUSTER QUEUE                   CQLOAD   USED    RES  AVAIL  TOTAL aoACDS  cdsuE  
--------------------------------------------------------------------------------
all.q                             0.12      0      0      1      1      0      0 
ENDOUTPUT

args="qstat -u * -s rs"
output=""
args="qstat -u * -s p"
output=""

# TODO: Below output is NOT from sge tools, but have been adapted to SGEmod code. Please provide working output.
args="qhost -xml"
output=<<<ENDOUTPUT
<qhost>
<host name="node1.site.org"/>
</qhost>
ENDOUTPUT

args="qstat -f"
output="host=node1.site.org"

args="qhost -F -h node1"
output="host=node1.site.org"
ENDSIMULATOROUTPUT

  my $cfg = {  
	           sge_bin_path => "<TESTDIR>/bin",
               sge_root => "<TESTDIR>/bin",
               queues => {},
               jobs => [],
               loglevel => '5'
            };
  $cfg->{queues}{'all.q'} = {users => ['user1']};
  
  my $lrms_info = $suite->collect(\@progs, $simulator_output, $cfg);
  
  is(ref $lrms_info, 'HASH', 'result type');
  is(ref $lrms_info->{cluster}, 'HASH', 'has cluster');
  is($lrms_info->{cluster}{lrms_type}, 'SGE', 'lrms_type');
  is($lrms_info->{cluster}{lrms_version}, '6.2', 'lrms_version');
  is($lrms_info->{cluster}{runningjobs}, 0, 'runningjobs');
  #is($lrms_info->{cluster}{totalcpus}, 1, 'totalcpus');
  is($lrms_info->{cluster}{queuedcpus}, 0, 'queuedcpus');
  is($lrms_info->{cluster}{queuedjobs}, 0, 'queuedjobs');
  is($lrms_info->{cluster}{usedcpus}, 0, 'usedcpus');
  #is($lrms_info->{cluster}{cpudistribution}, '1cpu:1', 'cpudistribution');
  
  is(ref $lrms_info->{queues}, 'HASH', 'has queues');
  is(ref $lrms_info->{queues}{'all.q'}, 'HASH', 'has queue all.q');
  ok($lrms_info->{queues}{'all.q'}{status} >= 0, 'all.q->status');
  is($lrms_info->{queues}{'all.q'}{totalcpus}, 1, 'all.q->totalcpus');
  is($lrms_info->{queues}{'all.q'}{queued}, 0, 'all.q->queued');
  is($lrms_info->{queues}{'all.q'}{running}, 0, 'all.q->running');
  is($lrms_info->{queues}{'all.q'}{maxrunning}, 1, 'all.q->maxrunning');
  
  is(ref $lrms_info->{queues}{'all.q'}{users}, 'HASH', 'has users');
  is(ref $lrms_info->{queues}{'all.q'}{users}{user1}, 'HASH', 'has user1');
  is($lrms_info->{queues}{'all.q'}{users}{user1}{queuelength}, 0, 'user1->queuelenght');
  is_deeply($lrms_info->{queues}{'all.q'}{users}{user1}{freecpus}, { '1' => 0 }, 'user1->freecpus');
  
  #is(ref $lrms_info->{nodes}, 'HASH', 'has nodes');
  #is(ref $lrms_info->{nodes}{node1}, 'HASH', 'has node1');
  #is($lrms_info->{nodes}{node1}{isavailable}, 1, 'node1->isavailable');
  #is($lrms_info->{nodes}{node1}{slots}, 2, 'node1->slots');
});

$suite->testing_done();
