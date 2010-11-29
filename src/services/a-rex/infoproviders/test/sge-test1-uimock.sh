#!/bin/bash
  
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


cmd="$(basename $0) $*"

case $cmd in

#######################################
  'qstat -help')
    cat <<ENDL
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
ENDL
    exit 0
    ;;

#######################################
  "qstat -u * -f")
    cat <<ENDF
queuename                      qtype resv/used/tot. load_avg arch          states
---------------------------------------------------------------------------------
all.q@node1.site.org           BIP   0/0/1          0.17     lx24-amd64    
ENDF
    exit 0
    ;;

#######################################
  'qconf -sep')
    cat <<ENDF
HOST                      PROCESSOR        ARCH
===============================================
node1.site.org                    1  lx24-amd64
===============================================
SUM                               1
ENDF
    exit 0
    ;;

#######################################
  'qconf -sconf global')
    cat <<ENDF
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

ENDF
    exit 0
    ;;

#######################################
  'qconf -sql')
    cat <<ENDF
all.q
ENDF
    exit 0
    ;;

#######################################
  'qconf -sq all.q')
    cat <<ENDF
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
ENDF
    exit 0
    ;;

#######################################
  'qstat -j')
    cat <<ENDF
scheduling info:            (Collecting of scheduler job information is turned off)

ENDF
    exit 0
    ;;

#######################################
  'qstat -g c')
    cat <<ENDF
CLUSTER QUEUE                   CQLOAD   USED    RES  AVAIL  TOTAL aoACDS  cdsuE  
--------------------------------------------------------------------------------
all.q                             0.12      0      0      1      1      0      0 
ENDF
    exit 0
    ;;

#######################################
  'qstat -u * -s rs')
    cat <<ENDF
ENDF
    exit 0
    ;;

#######################################
  'qstat -u * -s p')
    cat <<ENDF
ENDF
    exit 0
    ;;

#######################################
  *)
    echo $0: No mock behavior implemented for: $* 2>&1
    exit 127
    ;;
esac
