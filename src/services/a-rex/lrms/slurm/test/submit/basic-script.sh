#!/bin/bash -l
# SLURM batch job script built by arex
#SBATCH --no-requeue
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
# Setting environment variables as specified by user
export 'GRID_GLOBAL_JOBID=@TEST_JOB_ID@'

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
      if ! mv "$f" "$RUNTIME_NODE_JOB_DIR"; then
        echo "Failed to move '$f' to '$RUNTIME_NODE_JOB_DIR'" 1>&2
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
nodename=`/bin/hostname -f`
echo "nodename=$nodename" >> "$RUNTIME_JOB_DIAG"
echo "Processors=1" >> "$RUNTIME_JOB_DIAG"
executable='/bin/true'
# Check if executable exists
if [ ! -f "$executable" ]; 
then 
  echo "Path \"$executable\" does not seem to exist" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1
fi
# See if executable is a script, and extract the name of the interpreter
line1=`dd if="$executable" count=1 2>/dev/null | head -n 1`
command=`echo $line1 | sed -n 's/^#! *//p'`
interpreter=`echo $command | awk '{print $1}'`
if [ "$interpreter" = /usr/bin/env ]; then interpreter=`echo $command | awk '{print $2}'`; fi
# If it's a script and the interpreter is not found ...
[ "x$interpreter" = x ] || type "$interpreter" > /dev/null 2>&1 || {

  echo "Cannot run $executable: $interpreter: not found" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1; }
GNU_TIME='/usr/bin/time'
if [ ! -z "$GNU_TIME" ] && ! "$GNU_TIME" --version >/dev/null 2>&1; then
  echo "WARNING: GNU time not found at: $GNU_TIME" 2>&1;
  GNU_TIME=
fi 

if [ -z "$GNU_TIME" ] ; then
   "/bin/true" <$RUNTIME_JOB_STDIN 1>$RUNTIME_JOB_STDOUT 2>&1
else
  $GNU_TIME -o "$RUNTIME_JOB_DIAG" -a -f 'WallTime=%es\nKernelTime=%Ss\nUserTime=%Us\nCPUUsage=%P\nMaxResidentMemory=%MkB\nAverageResidentMemory=%tkB\nAverageTotalMemory=%KkB\nAverageUnsharedMemory=%DkB\nAverageUnsharedStack=%pkB\nAverageSharedMemory=%XkB\nPageSize=%ZB\nMajorPageFaults=%F\nMinorPageFaults=%R\nSwaps=%W\nForcedSwitches=%c\nWaitSwitches=%w\nInputs=%I\nOutputs=%O\nSocketReceived=%r\nSocketSent=%s\nSignals=%k\n'  "/bin/true" <$RUNTIME_JOB_STDIN 1>$RUNTIME_JOB_STDOUT 2>&1

fi
RESULT=$?

fi
fi
# Running RTE scripts (stage 2)
runtimeenvironments=
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
  echo "exitcode=$RESULT" >> "$RUNTIME_JOB_DIAG"
  exit $RESULT
