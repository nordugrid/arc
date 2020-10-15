#!/bin/bash -l
# SLURM batch job script built by arex
#SBATCH --no-requeue
#SBATCH --export=NONE
#SBATCH -e @TEST_SESSION_DIR@/@TEST_JOB_ID@.comment
#SBATCH -o @TEST_SESSION_DIR@/@TEST_JOB_ID@.comment

#SBATCH --nice=50
#SBATCH -J 'gridjob'
#SBATCH --get-user-env=10L
#SBATCH -n 1
#SBATCH 

# Overide umask of execution node (sometime values are really strange)
umask 077
 
# source with arguments for DASH shells
sourcewithargs() {
script=$1
shift
. $script
}
echo "Detecting resource accounting method available for the job." 1>&2
JOB_ACCOUNTING=""
# Try to use cgroups first
if command -v arc-job-cgroup >/dev/null 2>&1; then
    echo "Found arc-job-cgroup tool: trying to initialize accounting cgroups for the job." 1>&2
    while true; do
        # memory cgroup
        memory_cgroup="$( arc-job-cgroup -m -n @TEST_JOB_ID@ )"
        if [ $? -ne 0 -o -z "$memory_cgroup" ]; then
            echo "Failed to initialize memory cgroup for accounting." 1>&2
            break;
        fi
        # cpuacct cgroup
        cpuacct_cgroup="$( arc-job-cgroup -c -n @TEST_JOB_ID@ )"
        if [ $? -ne 0 -o -z "$cpuacct_cgroup" ]; then
            echo "Failed to initialize cpuacct cgroup for accounting." 1>&2
            break;
        fi
        echo "Using cgroups method for job accounting" 1>&2
        JOB_ACCOUNTING="cgroup"
        break;
    done
fi
# Fallback to GNU_TIME if cgroups are not working
if [ -z "$JOB_ACCOUNTING" ]; then
    GNU_TIME='/usr/bin/time'
    echo "Looking for $GNU_TIME tool for accounting measurements" 1>&2
    if [ ! -z "$GNU_TIME" ] && ! "$GNU_TIME" --version >/dev/null 2>&1; then
        echo "GNU time is not found at: $GNU_TIME" 1>&2
    else
        echo "GNU time found and will be used for job accounting." 1>&2
        JOB_ACCOUNTING="gnutime"
    fi
fi
# Nothing works: rely on LRMS only
if [ -z "$JOB_ACCOUNTING" ]; then
    echo "Failed to use both cgroups and GNU time for resource usage accounting. Accounting relies on LRMS information only." 1>&2
fi
# Setting environment variables as specified by user
export 'GRID_GLOBAL_JOBID=@TEST_JOB_ID@'
export 'GRID_GLOBAL_JOBURL='
export 'GRID_GLOBAL_JOBINTERFACE='
export 'GRID_GLOBAL_JOBHOST='

RUNTIME_JOB_DIR=@TEST_SESSION_DIR@/@TEST_JOB_ID@
RUNTIME_JOB_STDIN=/dev/null
RUNTIME_JOB_STDOUT=/dev/null
RUNTIME_JOB_STDERR=/dev/null
RUNTIME_JOB_DIAG=@TEST_SESSION_DIR@/@TEST_JOB_ID@.diag
if [ ! -z "$RUNTIME_GRIDAREA_DIR" ] ; then
  RUNTIME_JOB_DIR=$RUNTIME_GRIDAREA_DIR/`basename $RUNTIME_JOB_DIR`
  RUNTIME_JOB_STDIN=`echo "$RUNTIME_JOB_STDIN" | sed "s#^$RUNTIME_JOB_DIR#$RUNTIME_GRIDAREA_DIR#"`
  RUNTIME_JOB_STDOUT=`echo "$RUNTIME_JOB_STDOUT" | sed "s#^$RUNTIME_JOB_DIR#$RUNTIME_GRIDAREA_DIR#"`
  RUNTIME_JOB_STDERR=`echo "$RUNTIME_JOB_STDERR" | sed "s#^$RUNTIME_JOB_DIR#$RUNTIME_GRIDAREA_DIR#"`
  RUNTIME_JOB_DIAG=`echo "$RUNTIME_JOB_DIAG" | sed "s#^$RUNTIME_JOB_DIR#$RUNTIME_GRIDAREA_DIR#"`
  RUNTIME_CONTROL_DIR=`echo "$RUNTIME_CONTROL_DIR" | sed "s#^$RUNTIME_JOB_DIR#$RUNTIME_GRIDAREA_DIR#"`
fi
RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-}
RUNTIME_LOCAL_SCRATCH_MOVE_TOOL=${RUNTIME_LOCAL_SCRATCH_MOVE_TOOL:-mv}
RUNTIME_FRONTEND_SEES_NODE=${RUNTIME_FRONTEND_SEES_NODE:-}
RUNTIME_NODE_SEES_FRONTEND=${RUNTIME_NODE_SEES_FRONTEND:-yes}
  if [ ! -z "$RUNTIME_LOCAL_SCRATCH_DIR" ] && [ ! -z "$RUNTIME_NODE_SEES_FRONTEND" ]; then
    RUNTIME_NODE_JOB_DIR="$RUNTIME_LOCAL_SCRATCH_DIR"/`basename "$RUNTIME_JOB_DIR"`
    rm -rf "$RUNTIME_NODE_JOB_DIR"
    mkdir -p "$RUNTIME_NODE_JOB_DIR"
    # move directory contents
    for f in "$RUNTIME_JOB_DIR"/.* "$RUNTIME_JOB_DIR"/*; do 
      [ "$f" = "$RUNTIME_JOB_DIR/*" ] && continue # glob failed, no files
      [ "$f" = "$RUNTIME_JOB_DIR/." ] && continue
      [ "$f" = "$RUNTIME_JOB_DIR/.." ] && continue
      [ "$f" = "$RUNTIME_JOB_DIR/.diag" ] && continue
      [ "$f" = "$RUNTIME_JOB_DIR/.comment" ] && continue
      if ! $RUNTIME_LOCAL_SCRATCH_MOVE_TOOL "$f" "$RUNTIME_NODE_JOB_DIR"; then
        echo "Failed to '$RUNTIME_LOCAL_SCRATCH_MOVE_TOOL' '$f' to '$RUNTIME_NODE_JOB_DIR'" 1>&2
        exit 1
      fi
    done
    if [ ! -z "$RUNTIME_FRONTEND_SEES_NODE" ] ; then
      # creating link for whole directory
       ln -s "$RUNTIME_FRONTEND_SEES_NODE"/`basename "$RUNTIME_JOB_DIR"` "$RUNTIME_JOB_DIR"
    else
      # keep stdout, stderr and control directory on frontend
      # recreate job directory
      mkdir -p "$RUNTIME_JOB_DIR"
      # make those files
      mkdir -p `dirname "$RUNTIME_JOB_STDOUT"`
      mkdir -p `dirname "$RUNTIME_JOB_STDERR"`
      touch "$RUNTIME_JOB_STDOUT"
      touch "$RUNTIME_JOB_STDERR"
      RUNTIME_JOB_STDOUT__=`echo "$RUNTIME_JOB_STDOUT" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
      RUNTIME_JOB_STDERR__=`echo "$RUNTIME_JOB_STDERR" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
      rm "$RUNTIME_JOB_STDOUT__" 2>/dev/null
      rm "$RUNTIME_JOB_STDERR__" 2>/dev/null
      if [ ! -z "$RUNTIME_JOB_STDOUT__" ] && [ "$RUNTIME_JOB_STDOUT" != "$RUNTIME_JOB_STDOUT__" ]; then
        ln -s "$RUNTIME_JOB_STDOUT" "$RUNTIME_JOB_STDOUT__"
      fi
      if [ "$RUNTIME_JOB_STDOUT__" != "$RUNTIME_JOB_STDERR__" ] ; then
        if [ ! -z "$RUNTIME_JOB_STDERR__" ] && [ "$RUNTIME_JOB_STDERR" != "$RUNTIME_JOB_STDERR__" ]; then
          ln -s "$RUNTIME_JOB_STDERR" "$RUNTIME_JOB_STDERR__"
        fi
      fi
      if [ ! -z "$RUNTIME_CONTROL_DIR" ] ; then
        # move control directory back to frontend
        RUNTIME_CONTROL_DIR__=`echo "$RUNTIME_CONTROL_DIR" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
        mv "$RUNTIME_CONTROL_DIR__" "$RUNTIME_CONTROL_DIR"
      fi
    fi
    # adjust stdin,stdout & stderr pointers
    RUNTIME_JOB_STDIN=`echo "$RUNTIME_JOB_STDIN" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
    RUNTIME_JOB_STDOUT=`echo "$RUNTIME_JOB_STDOUT" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
    RUNTIME_JOB_STDERR=`echo "$RUNTIME_JOB_STDERR" | sed "s#^${RUNTIME_JOB_DIR}#${RUNTIME_NODE_JOB_DIR}#"`
    RUNTIME_FRONTEND_JOB_DIR="$RUNTIME_JOB_DIR"
    RUNTIME_JOB_DIR="$RUNTIME_NODE_JOB_DIR"
  fi
  if [ -z "$RUNTIME_NODE_SEES_FRONTEND" ] ; then
    mkdir -p "$RUNTIME_JOB_DIR"
  fi

RESULT=0

if [ "$RESULT" = '0' ] ; then
# Running RTE scripts (stage 1)
runtimeenvironments=
echo "runtimeenvironments=$runtimeenvironments" >> "$RUNTIME_JOB_DIAG"
if [ ! "X$SLURM_NODEFILE" = 'X' ] ; then
  if [ -r "$SLURM_NODEFILE" ] ; then
    cat "$SLURM_NODEFILE" | sed 's/\(.*\)/nodename=\1/' >> "$RUNTIME_JOB_DIAG"
    NODENAME_WRITTEN="1"
  else
    SLURM_NODEFILE=
  fi
fi
# Detecting WN operating system for accounting purposes
if [ -f "/etc/os-release" ]; then
  SYSTEM_SOFTWARE="$( eval $( cat /etc/os-release ); echo "${NAME} ${VERSION}" )"
elif [ -f "/etc/system-release" ]; then
  SYSTEM_SOFTWARE="$( cat /etc/system-release )"
elif command -v lsb_release >/dev/null 2>&1; then
  SYSTEM_SOFTWARE=$(lsb_release -ds)
elif command -v hostnamectl >/dev/null 2>&1; then
  SYSTEM_SOFTWARE="$( hostnamectl 2>/dev/null | sed -n '/Operating System/s/^\s*Operating System:\s*//p' )"
elif command -v uname >/dev/null 2>&1; then
  SYSTEM_SOFTWARE="Linux $( uname -r)"
fi
[ -n "$SYSTEM_SOFTWARE" ] && echo "systemsoftware=${SYSTEM_SOFTWARE}" >> "$RUNTIME_JOB_DIAG"

if [ "$RESULT" = '0' ] ; then
  # Changing to session directory
  HOME=$RUNTIME_JOB_DIR
  export HOME
  if ! cd "$RUNTIME_JOB_DIR"; then
    echo "Failed to switch to '$RUNTIME_JOB_DIR'" 1>&2
    RESULT=1
  fi
  if [ ! -z "$RESULT" ] && [ "$RESULT" != 0 ]; then
    exit $RESULT
  fi
# Write nodename if not already written in LRMS-specific way
if [ -z "$NODENAME_WRITTEN" ] ; then
  nodename=`/bin/hostname -f`
  echo "nodename=$nodename" >> "$RUNTIME_JOB_DIAG"
fi
echo "Processors=1" >> "$RUNTIME_JOB_DIAG"
echo "Benchmark=HEPSPEC:1.0" >> "$RUNTIME_JOB_DIAG"
executable='/bin/true'
# Check if executable exists
if [ ! -f "$executable" ]; 
then 
  echo "Path \"$executable\" does not seem to exist" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1
fi
# See if executable is a script, and extract the name of the interpreter
line1=`dd if="$executable" count=1 2>/dev/null | head -n 1`
shebang=`echo $line1 | sed -n 's/^#! *//p'`
interpreter=`echo $shebang | awk '{print $1}'`
if [ "$interpreter" = /usr/bin/env ]; then interpreter=`echo $shebang | awk '{print $2}'`; fi
# If it's a script and the interpreter is not found ...
[ "x$interpreter" = x ] || type "$interpreter" > /dev/null 2>&1 || {

  echo "Cannot run $executable: $interpreter: not found" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1; }
if [ "x$JOB_ACCOUNTING" = "xgnutime" ]; then
  $GNU_TIME -o "$RUNTIME_JOB_DIAG" -a -f 'WallTime=%es\nKernelTime=%Ss\nUserTime=%Us\nCPUUsage=%P\nMaxResidentMemory=%MkB\nAverageResidentMemory=%tkB\nAverageTotalMemory=%KkB\nAverageUnsharedMemory=%DkB\nAverageUnsharedStack=%pkB\nAverageSharedMemory=%XkB\nPageSize=%ZB\nMajorPageFaults=%F\nMinorPageFaults=%R\nSwaps=%W\nForcedSwitches=%c\nWaitSwitches=%w\nInputs=%I\nOutputs=%O\nSocketReceived=%r\nSocketSent=%s\nSignals=%k\n'  "/bin/true" <$RUNTIME_JOB_STDIN 1>$RUNTIME_JOB_STDOUT 2>&1
else
   "/bin/true" <$RUNTIME_JOB_STDIN 1>$RUNTIME_JOB_STDOUT 2>&1
fi
RESULT=$?

fi
fi
# Running RTE scripts (stage 2)
runtimeenvironments=
# Measuring used scratch space
echo "usedscratch=$( du -sb "$RUNTIME_JOB_DIR" | sed "s/\s.*$//" )" >> "$RUNTIME_JOB_DIAG"
# Cleaning up extra files in the local scratch
if [ ! -z  "$RUNTIME_LOCAL_SCRATCH_DIR" ] ; then
  find ./ -type l -exec rm -f "{}" ";"
  chmod -R u+w "./"
  find ./ -type f -perm /200 -exec rm -f "{}" ";"
  chmod -R u+w "./"
fi

  if [ ! -z "$RUNTIME_LOCAL_SCRATCH_DIR" ] && [ ! -z "$RUNTIME_NODE_SEES_FRONTEND" ]; then 
    if [ ! -z "$RUNTIME_FRONTEND_SEES_NODE" ] ; then
      # just move it
      rm -rf "$RUNTIME_FRONTEND_JOB_DIR"
      destdir=`dirname "$RUNTIME_FRONTEND_JOB_DIR"`
      if ! mv "$RUNTIME_NODE_JOB_DIR" "$destdir"; then
        echo "Failed to move '$RUNTIME_NODE_JOB_DIR' to '$destdir'" 1>&2
        RESULT=1
      fi

else
      # remove links
      rm -f "$RUNTIME_JOB_STDOUT" 2>/dev/null
      rm -f "$RUNTIME_JOB_STDERR" 2>/dev/null
      # move directory contents
      for f in "$RUNTIME_NODE_JOB_DIR"/.* "$RUNTIME_NODE_JOB_DIR"/*; do 
        [ "$f" = "$RUNTIME_NODE_JOB_DIR/*" ] && continue # glob failed, no files
        [ "$f" = "$RUNTIME_NODE_JOB_DIR/." ] && continue
        [ "$f" = "$RUNTIME_NODE_JOB_DIR/.." ] && continue
        [ "$f" = "$RUNTIME_NODE_JOB_DIR/.diag" ] && continue
        [ "$f" = "$RUNTIME_NODE_JOB_DIR/.comment" ] && continue
        if ! mv "$f" "$RUNTIME_FRONTEND_JOB_DIR"; then
          echo "Failed to move '$f' to '$RUNTIME_FRONTEND_JOB_DIR'" 1>&2
          RESULT=1
        fi
      done
      rm -rf "$RUNTIME_NODE_JOB_DIR"
    fi
  fi
# Handle cgroup measurements
if [ "x$JOB_ACCOUNTING" = "xcgroup" ]; then
    # Max memory used (total)
    maxmemory=$( cat "${memory_cgroup}/memory.memsw.max_usage_in_bytes" )
    maxmemory=$(( (maxmemory + 1023) / 1024 ))
    echo "maxtotalmemory=${maxmemory}kB" >> "$RUNTIME_JOB_DIAG"

    # Max memory used (RAM)
    maxram=$( cat "${memory_cgroup}/memory.max_usage_in_bytes" )
    maxram=$(( (maxram + 1023) / 1024 ))
    echo "maxresidentmemory=${maxram}kB" >> "$RUNTIME_JOB_DIAG"
    # TODO: this is for compatibilty with current A-REX accounting code. Remove when A-REX will use max value instead.
    echo "averageresidentmemory=${maxram}kB" >> "$RUNTIME_JOB_DIAG"

    # User CPU time
    if [ -f "${cpuacct_cgroup}/cpuacct.usage_user" ]; then
        # cgroup values are in nanoseconds
        user_cputime=$( cat "${cpuacct_cgroup}/cpuacct.usage_user" )
        user_cputime=$(( user_cputime / 1000000 ))
    elif [ -f "${cpuacct_cgroup}/cpuacct.stat" ]; then
        # older kernels have only cpuacct.stat that use USER_HZ units
        user_cputime=$( cat "${cpuacct_cgroup}/cpuacct.stat" | sed -n '/^user/s/user //p' )
        user_hz=$( getconf CLK_TCK )
        user_cputime=$(( user_cputime / user_hz ))
    fi
    [ -n "$user_cputime" ] && echo "usertime=${user_cputime}" >> "$RUNTIME_JOB_DIAG"

    # Kernel CPU time
    if [ -f "${cpuacct_cgroup}/cpuacct.usage_sys" ]; then
        # cgroup values are in nanoseconds
        kernel_cputime=$( cat "${cpuacct_cgroup}/cpuacct.usage_sys" )
        kernel_cputime=$(( kernel_cputime / 1000000 ))
    elif [ -f "${cpuacct_cgroup}/cpuacct.stat" ]; then
        # older kernels have only cpuacct.stat that use USER_HZ units
        kernel_cputime=$( cat "${cpuacct_cgroup}/cpuacct.stat" | sed -n '/^system/s/system //p' )
        [ -z "$user_hz" ] && user_hz=$( getconf CLK_TCK )
        kernel_cputime=$(( kernel_cputime / user_hz ))
    fi
    [ -n "$kernel_cputime" ] && echo "kerneltime=${kernel_cputime}" >> "$RUNTIME_JOB_DIAG"

    # Remove nested job accouting cgroups
    arc-job-cgroup -m -d
    arc-job-cgroup -c -d
fi

# Record CPU benchmarking values for WN user by the job
[ -n "${ACCOUNTING_BENCHMARK}" ] && echo "benchmark=${ACCOUNTING_BENCHMARK}" >> "$RUNTIME_JOB_DIAG"
# Record WN instance tag if defined
[ -n "${ACCOUNTING_WN_INSTANCE}" ] && echo "wninstance=${ACCOUNTING_WN_INSTANCE}" >> "$RUNTIME_JOB_DIAG"

# Add exit code to the accounting information and exit the job script
echo "exitcode=$RESULT" >> "$RUNTIME_JOB_DIAG"
exit $RESULT
