# This file contains functions that are used througout the scan-*-job scripts.

progname=$(basename "$0")

#
# scan-*-jobs has STDOUT redirected to /dev/null and STDERR redirected to
# helperlog
#
log () { echo "[`date +%Y-%m-%d\ %T`] $progname: $*" 1>&2; }

common_init () {
   # parse configuration
   parse_arc_conf
   # read pbs-specific environment
   . ${pkgdatadir}/configure-${joboption_lrms}-env.sh || exit $?
   # set common LRMS environmental variables
   init_lrms_env
}

perflog_common () {
   perflog_dname=$1
   d=`date +%F`
   perflog_fname=${perflog_dname}/system${d}.perflog
   jobstatus_dir=$2
   #gather performance information
   loadavg=`cat /proc/loadavg`
   memtotal=$(a=`grep MemTotal /proc/meminfo`; echo ${a#MemTotal:})
   memfree=$(a=`grep MemFree /proc/meminfo`; echo ${a#MemFree:})
   if [ -d "$jobstatus_dir" ]; then
      jsd_size=`ls -l $jobstatus_dir| wc -l`
      jsdP_size=`ls -l $jobstatus_dir/processing | wc -l`
   fi
   #log the loadavg, stripping the last elemenmt, and the rest of the gathered info
   echo "[`date +%Y-%m-%d\ %T`] LoadAVG: ${loadavg% *}" >> $perflog_fname;
   echo "[`date +%Y-%m-%d\ %T`] MemStat: $memtotal $memfree" >> $perflog_fname;
   echo "[`date +%Y-%m-%d\ %T`] Control dir: $jsd_size $jsdP_size" >> $perflog_fname;
   # gather gridftp info
   gftp_pid=`cat /run/gridftpd.pid`
   gsiftp=`top -b -n 1 -p ${gftp_pid} | grep -w ${gftp_pid} | sed -e 's/[[:space:]]*$//'`
   printf "[`date +%Y-%m-%d\ %T`] Gridftpd: $gsiftp\n" >> $perflog_fname;
   # gather slapd info
   slapd_pid=`cat /run/arc/bdii/db/slapd.pid`
   slapd=`top -b -n 1 -p ${slapd_pid} | grep -w ${slapd_pid} | sed -e 's/[[:space:]]*$//'`
   printf "[`date +%Y-%m-%d\ %T`] Slapd: ${slapd}\n" >> $perflog_fname;
   # gather a-rex information
   arex_pid=`cat /run/arched-arex.pid`
   arex=`top -b -n 1 -p ${arex_pid} | grep -w ${arex_pid} | sed -e 's/[[:space:]]*$//'`
   prinft "[`date +%Y-%m-%d\ %T`] A-Rex: ${arex}\n" >> $perflog_fname;
 
   unset perflog_dname
   unset perflog_fname
   unset jobstatus_dir
}

# This function takes a time interval formatted as 789:12:34:56 (with days) or
# 12:34:56 (without days) and transforms it to seconds. It returns the result in
# the return_interval_seconds variable.
interval_to_seconds () {
    _interval_dhms=$1
    _interval_size=`echo $_interval_dhms | grep -o : | wc -l`
    if [ $_interval_size -eq 2 ]; then
        return_interval_seconds=`echo $_interval_dhms | tr : ' ' | awk '{print $1*60*60+$2*60+$3;}'`
    elif [ $_interval_size -eq 3 ]; then
        return_interval_seconds=`echo $_interval_dhms | tr : ' ' | awk '{print $1*24*60*60+$2*60*60+$3*60+$4;}'`
    else
        echo "Bad formatting of time interval: $_interval_dhms" >&2
        return_interval_seconds=
    fi
    unset _interval_dhms _interval_size
}

# This function takes a date string in the form recognized by the date utility
# and transforms it into seconds in UTC time.  It returns the result in the
# return_date_seconds variable.
date_to_utc_seconds () {
    _date_string=$1
    return_date_seconds=
    [ -z "$_date_string" ] && return
    _date_seconds=`date -d "$_date_string" +%s`
    [ ! $? = 0 ] && return
    date_seconds_to_utc "$_date_seconds"
    unset _date_string _date_seconds
}

# This function takes a timestamp as seconds in local time and transforms it into
# seconds in UTC time.  It returns the result in the return_date_seconds variable.
date_seconds_to_utc () {
    _date_seconds=$1
    _offset_hms=`date +"%::z"`
    _offset_seconds=`echo $_offset_hms | tr ':' ' ' | awk '{ print $1*60*60+$2*60+$3; }'`
    return_date_seconds=$(( $_date_seconds - ($_offset_seconds) ))
    unset _date_seconds _offset_hms _offset_seconds
}

# This function takes a timestamp as seconds and transforms it to Mds date
# format (YYYYMMDDHHMMSSZ).  It returns the result in the return_mds_date
# variable.
seconds_to_mds_date () {
    _date_seconds=$1
    return_mds_date=`date -d "1970-01-01 UTC $_date_seconds seconds" +"%Y%m%d%H%M%SZ"`
    unset _date_seconds
}

#
# gets the numerical uid of the owner of a file
#
get_owner_uid () {
  script='my $filename = $ARGV[0];
          exit 1 unless $filename;
          my @stat = stat($ARGV[0]);
          exit 1 unless defined $stat[4];
          print "$stat[4]\n";
         '
  /usr/bin/perl -we "$script" "$1"
}

#
# If running as root, attempts to switch to the uid passed as the first
# argument and then runs the command passed as the second argument in a shell.
# The remaining arguments are passed as arguments to the shell.
#
do_as_uid () {
    test $# -ge 2 || { log "do_as_uid requires 2 arguments"; return 1; }

    script='use English;
            my ($uid, @args) = @ARGV;
            if ( $UID == 0 ) {
                my ($name, $pass, $uid, $gid, $quota, $comment, $gcos, $dir, $shell, $expire) = getpwuid($uid);
                eval { 
                    $GID = $gid;
                    $UID = $uid };
                print STDERR "Cannot switch to uid($UID): $@\n" if $@;
            }
            system("'$POSIX_SHELL'","-c",@args);
            exit 0 if $? eq 0;
            exit ($?>>8||128+($?&127));
    '
    /usr/bin/perl -we "$script" "$@"
}

#
# Input variables:
#   * sessiondir
#   * uid
# Output variables:
#   * diagstring -- the whole contents of .diag
#   * nodename
#   * WallTime
#   * UserTime
#   * KernelTime
#   * TotalMemory
#   * ResidentMemory
#   * LRMSStartTime
#   * LRMSEndTime
#   * exitcode
#
job_read_diag() {

    [ -n "$uid" ] && [ -n "$sessiondir" ] \
    || { log "job_read_diag requires the following to be set: uid sessiondir"; return 1; }

    diagfile=$sessiondir.diag;
    [ -f "$diagfile" ] || { log "diag file not found at: $sessiondir.diag"; return 1; }

    diagstring=$(do_as_uid $uid "tail -n 1000 '$diagfile'")
    [ $? = 0 ] || { log "cannot read diag file at: $diagfile"; return 1; }

    nodename=$(echo "$diagstring" | sed -n 's/^nodename=\(..*\)/\1/p')
    WallTime=$(echo "$diagstring" | sed -n 's/^WallTime=\([0-9.]*\)s/\1/p' | tail -n 1)
    UserTime=$(echo "$diagstring" | sed -n 's/^UserTime=\([0-9.]*\)s/\1/p' | tail -n 1)
    KernelTime=$(echo "$diagstring" | sed -n 's/^KernelTime=\([0-9.]*\)s/\1/p' | tail -n 1)
    TotalMemory=$(echo "$diagstring" | sed -n 's/^AverageTotalMemory=\([0-9.]*\)kB/\1/p' | tail -n 1)
    ResidentMemory=$(echo "$diagstring" | sed -n 's/^AverageResidentMemory=\([0-9.]*\)kB/\1/p' | tail -n 1)
    LRMSStartTime=$(echo "$diagstring" | sed -n 's/^LRMSStartTime=\([0-9][0-9]*Z\)/\1/p' | tail -n 1)
    LRMSEndTime=$(echo "$diagstring" | sed -n 's/^LRMSEndTime=\([0-9][0-9]*Z\)/\1/p' | tail -n 1)
    exitcode=$(echo "$diagstring" | sed -n 's/^exitcode=\([0-9]*\)/\1/p' | tail -n 1)

    for key in nodename WallTime UserTime KernelTime AverageTotalMemory AverageResidentMemory \
               exitcode LRMSStartTime LRMSEndTime LRMSExitcode LRMSMessage; do
        diagstring=$(echo "$diagstring" | grep -v "^$key=")
    done
}

#
# Input variables:
#   * sessiondir
#   * uid
#   * LRMSExitcode
#   * LRMSMessage
#   + all output variables from job_read_diag
# OBS: nodename should be a multi-line string, one line per node (or is it per cpu used?)
# OBS: UserTime, KernelTime, Walltime must be given in seconds (without unit at the end)
# OBS: TotalMemory, ResidentMemory must be given in kB (without unit at the end)
# OBS: LRMSStartTime, LRMSEndTime must be of Mds form YYYYMMDDHHMMSSZ (note: UTC timezone)
#
job_write_diag() {

    [ -n "$uid" ] && [ -n "$sessiondir" ] \
    || { log "job_write_diag requires the following to be set: uid sessiondir"; return 1; }

    diagfile=$sessiondir.diag;

    { echo "$diagstring" && echo
      [ -n "$nodename" ] && echo "$nodename" | sed -n 's/^\(..*\)/nodename=\1/p'
      [ -n "$WallTime" ] && echo "WallTime=${WallTime}s"
      [ -n "$Processors" ] && echo "Processors=${Processors}"
      [ -n "$UserTime" ] && echo "UserTime=${UserTime}s"
      [ -n "$KernelTime" ] && echo "KernelTime=${KernelTime}s"
      [ -n "$TotalMemory" ] && echo "AverageTotalMemory=${TotalMemory}kB"
      [ -n "$ResidentMemory" ] && echo "AverageResidentMemory=${ResidentMemory}kB"
      [ -n "$LRMSStartTime" ] && echo "LRMSStartTime=$LRMSStartTime"
      [ -n "$LRMSEndTime" ] && echo "LRMSEndTime=$LRMSEndTime"
      [ -n "$LRMSMessage" ] && echo "LRMSMessage=$LRMSMessage"
      [ -n "$LRMSExitcode" ] && echo "LRMSExitcode=$LRMSExitcode"
      [ -n "$exitcode" ] && echo "exitcode=$exitcode"
    } | do_as_uid $uid "cat > '$diagfile'"
    [ $? = 0 ] || { log "cannot write diag file at: $diagfile"; return 1; }
}

# Append .comment (containing STDOUT & STDERR of the job wrapper) to .errors
# This file can also contain a message from the LRMS (i.e. the reason for killing the job).
save_commentfile () {
  uid=$1
  commentfile=$2
  errorsfile=$3
  action="
    { echo '------- Contents of output stream forwarded by the LRMS ---------'
      cat '$commentfile' 2> /dev/null
      echo '------------------------- End of output -------------------------'
    } >> '$errorsfile'
  "
  do_as_uid "$uid" "$action"
}
