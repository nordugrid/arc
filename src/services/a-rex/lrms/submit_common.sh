######################################################
#Common functions for submit scripts
######################################################

init () {
   # Only CPU time specified in job limits, rough limit for wall time
   walltime_ratio='1'
   # Use specified CPU time as soft limit, allow to run a bit longer before hard limit
   time_hardlimit_ratio='1/1'
   # Use specified memory requirement as soft limit, allow a bit more before hard limit
   memory_hardlimit_ratio='1/1'
   # Where to store temporary files on gatekeeper
   TMP_DIR=${TMP_DIR:-/tmp}
   # Where GNU time utility is located on computing nodes (empty if does not exist)
   GNU_TIME=${GNU_TIME:-/usr/bin/time}
   # Command to get name of executing node
   NODENAME=${NODENAME:-/bin/hostname -f}

   RUNTIME_LOCAL_SCRATCH_DIR=${RUNTIME_LOCAL_SCRATCH_DIR:-}
   RUNTIME_ARC_LOCATION="$ARC_LOCATION"
   RUNTIME_GLOBUS_LOCATION="$GLOBUS_LOCATION"
} 


parse_arg_file () {
   arg_file=$1
   if [ -z "$arg_file" ] ; then
      echo "Arguments file should be specified" 1>&2
      exit 1
   fi
   if [ ! -f $arg_file ] ; then
      echo "Missing arguments file" 1>&2
      exit 1
   fi
   . $arg_file

   echo "joboption_user=`whoami`" >> $arg_file

   if [ -z "$joboption_controldir" ] ; then
     joboption_controldir=`dirname "$arg_file"`
     if [ "$joboption_controldir" = '.' ] ; then
        joboption_controldir="$PWD"
     fi
   fi
   if [ -z "$joboption_gridid" ] ; then
     joboption_gridid=`basename "$arg_file" | sed 's/^job\.\(.*\)\.grami$/\1/'`
   fi

   ##############################################################
   # combine arguments to command -  easier to use
   ##############################################################
   i=0
   joboption_args=
   eval "var_is_set=\${joboption_arg_$i+yes}"
   while [ ! -z "${var_is_set}" ] ; do
      eval "var_value=\${joboption_arg_$i}"
      # Use -- to avoid echo eating arguments it understands
      var_value=`echo -- "$var_value" |cut -f2- -d' '| sed 's/\\\\/\\\\\\\\/' | sed 's/"/\\\"/'`
      joboption_args="$joboption_args \"${var_value}\""
      i=$(( i + 1 ))
      eval "var_is_set=\${joboption_arg_$i+yes}"
   done
}

##############################################################
# Zero stage of runtime environments
##############################################################

RTE_stage0 () {
   joboption_num=0
   eval "var_is_set=\${joboption_runtime_$joboption_num+yes}"
   while [ ! -z "${var_is_set}" ] ; do
      eval "var_value=\${joboption_runtime_$joboption_num}"
      if [ -r "$RUNTIME_CONFIG_DIR/${var_value}" ] ; then
         . "$RUNTIME_CONFIG_DIR/${var_value}" "0"
      else
         echo "Warning: runtime script ${var_value} is missing" 1>&2
      fi
      joboption_num=$(( joboption_num + 1 ))
      eval "var_is_set=\${joboption_runtime_$joboption_num+yes}"
   done
}

##############################################################
# create temp job script
##############################################################

mktempscript () {
   # File name to be used for temporary job script
   LRMS_JOB_SCRIPT=`mktemp ${TMP_DIR}/${joboption_lrms}_job_script.XXXXXX`
   echo "Created file $LRMS_JOB_SCRIPT"
   if [ -z "$LRMS_JOB_SCRIPT" ] ; then
      echo "Creation of temporary file failed"
      exit 1
   fi

   LRMS_JOB_OUT="${LRMS_JOB_SCRIPT}.out"
   touch $LRMS_JOB_OUT
   LRMS_JOB_ERR="${LRMS_JOB_SCRIPT}.err"
   touch $LRMS_JOB_ERR
   if [ ! -f "$LRMS_JOB_SCRIPT" ] || [ ! -f "$LRMS_JOB_OUT" ] || [ ! -f "$LRMS_JOB_ERR" ] ; then
      echo "Something is wrong. Either somebody is deleting files or I cannot write to ${TMP_DIR}"
      exit 1
   fi
}

##############################################################
# Execution times (obtained in seconds)
##############################################################

##############################################################
# Add environment variables
##############################################################

add_user_env () {
   echo "# Setting environment variables as specified by user" >> $LRMS_JOB_SCRIPT
   i=0
   eval "var_is_set=\${joboption_env_$i+yes}"
   while [ ! -z "${var_is_set}" ] ; do
      eval "var_value=\${joboption_env_$i}"
      echo "export ${var_value}" >> $LRMS_JOB_SCRIPT
      i=$(( i + 1 ))
      eval "var_is_set=\${joboption_env_$i+yes}"
   done
   echo "" >> $LRMS_JOB_SCRIPT
}

##############################################################
#  Runtime configuration
##############################################################
RTE_stage1 () {
   echo "# Running runtime scripts" >> $LRMS_JOB_SCRIPT
   echo "RUNTIME_CONFIG_DIR=\${RUNTIME_CONFIG_DIR:-$RUNTIME_CONFIG_DIR}" >> $LRMS_JOB_SCRIPT
   i=0
   eval "var_is_set=\${joboption_runtime_$i+yes}"
   echo "runtimeenvironments=" >> $LRMS_JOB_SCRIPT
   while [ ! -z "${var_is_set}" ] ; do
      if [ "$i" = '0' ] ; then
         echo "if [ ! -z \"\$RUNTIME_CONFIG_DIR\" ] ; then" >> $LRMS_JOB_SCRIPT
      fi
      eval "var_value=\"\${joboption_runtime_$i}\""
      echo "  if [ -r \"\${RUNTIME_CONFIG_DIR}/${var_value}\" ] ; then" >> $LRMS_JOB_SCRIPT
      echo "    runtimeenvironments=\"\${runtimeenvironments}${var_value};\"" >> $LRMS_JOB_SCRIPT
      echo "    source \${RUNTIME_CONFIG_DIR}/${var_value} 1 " >> $LRMS_JOB_SCRIPT
      echo "  fi" >> $LRMS_JOB_SCRIPT
      i=$(( i + 1 ))
      eval "var_is_set=\${joboption_runtime_$i+yes}"
   done

   if [ ! "$i" = '0' ] ; then
     echo "fi" >> $LRMS_JOB_SCRIPT
   fi

   echo "" >> $LRMS_JOB_SCRIPT

   gate_host=`uname -n`
   if [ -z "$gate_host" ] ; then
     echo "Can't get own hostname" 1>&2
     rm -f "$LRMS_JOB_SCRIPT" "$LRMS_JOB_OUT" "$LRMS_JOB_ERR"
     exit 1
   fi
}


#######################################################################
# copy information useful for transfering files to/from node directly
#######################################################################

setup_local_transfer () {
  RUNTIME_CONTROL_DIR=`mktemp ${joboption_directory}/control.XXXXXX`
  if [ -z "$RUNTIME_CONTROL_DIR" ] ; then
    echo 'Failed to choose name for temporary control directory' 1>&2
    exit 1
  fi
  rm -f "$RUNTIME_CONTROL_DIR"
  mkdir "$RUNTIME_CONTROL_DIR"
  if [ $? -ne '0' ] ; then
    echo 'Failed to create temporary control directory' 1>&2
    exit 1
  fi
  chmod go-rwx,u+rwx "${RUNTIME_CONTROL_DIR}"
  echo '' >"${RUNTIME_CONTROL_DIR}/job.local.proxy"
  chmod go-rw,u+r,a-x "${RUNTIME_CONTROL_DIR}/job.local.proxy"
  cat "${joboption_controldir}/job.${joboption_gridid}.proxy" >"${RUNTIME_CONTROL_DIR}/job.local.proxy"
  cat "${joboption_controldir}/job.${joboption_gridid}.input" >"${RUNTIME_CONTROL_DIR}/job.local.input"
  cat "${joboption_controldir}/job.${joboption_gridid}.output">"${RUNTIME_CONTROL_DIR}/job.local.output"
  RUNTIME_CONTROL_DIR_REL=`basename "$RUNTIME_CONTROL_DIR"`
  echo "$RUNTIME_CONTROL_DIR_REL *.*" >>"${RUNTIME_CONTROL_DIR}/job.local.input"
  echo "$RUNTIME_CONTROL_DIR_REL" >>"${RUNTIME_CONTROL_DIR}/job.local.output"
  echo "$RUNTIME_CONTROL_DIR_REL" >>"${joboption_controldir}/job.${joboption_gridid}.output"
  RUNTIME_STDOUT_REL=`echo "${joboption_stdout}" | sed "s#^${joboption_directory}##"`
  RUNTIME_STDERR_REL=`echo "${joboption_stderr}" | sed "s#^${joboption_directory}##"`
  echo "$RUNTIME_STDOUT_REL *.*" >>"${RUNTIME_CONTROL_DIR}/job.local.input"
  echo "$RUNTIME_STDERR_REL *.*" >>"${RUNTIME_CONTROL_DIR}/job.local.input"
  echo "RUNTIME_CONTROL_DIR=$RUNTIME_CONTROL_DIR" >> $LRMS_JOB_SCRIPT
}


#######################################################################
#uploading output files
#######################################################################

upload_output_files () {
   if [ "$joboption_localtransfer" = 'yes' ] ; then
     cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
     if [ "$RESULT" = '0' ] ; then
       $NORUDGRID_LOCATION/libexec/uploader -p -c 'local' "$RUNTIME_CONTROL_DIR" "$RUNTIME_JOB_DIR" 2>>${RUNTIME_CONTROL_DIR}/job..local.errors
       if [ $? -ne '0' ] ; then
         echo 'ERROR: Uploader failed.' 1>&2
         if [ "$RESULT" = '0' ] ; then RESULT=1 ; fi
       fi
     fi
     rm -f "${RUNTIME_CONTROL_DIR}/job.local.proxy"
EOSCR
   fi
}

#######################################################################
# downloading input files (this might fail for fork)
#######################################################################
download_input_files () {
  if [ "$joboption_localtransfer" = 'yes' ] ; then
    echo "ARC_LOCATION=\${ARC_LOCATION:-$RUNTIME_ARC_LOCATION}" >> $LRMS_JOB_SCRIPT
    echo "GLOBUS_LOCATION=\${GLOBUS_LOCATION:-$RUNTIME_GLOBUS_LOCATION}" >> $LRMS_JOB_SCRIPT
    cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
    if [ -z "$ARC_LOCATION" ] ; then
      echo 'Variable ARC_LOCATION is not set' 1>&2
      exit 1
    fi
    if [ -z "$GLOBUS_LOCATION" ] ; then
      echo 'Variable GLOBUS_LOCATION is not set' 1>&2
      exit 1
    fi
    export GLOBUS_LOCATION
    export ARC_LOCATION
    export LD_LIBRARY_PATH="$GLOBUS_LOCATION/lib:$LD_LIBRARY_PATH"
    export SASL_PATH="$GLOBUS_LOCATION/lib/sasl"
    export X509_USER_KEY="${RUNTIME_CONTROL_DIR}/job.local.proxy"
    export X509_USER_CERT="${RUNTIME_CONTROL_DIR}/job.local.proxy"
    export X509_USER_PROXY="${RUNTIME_CONTROL_DIR}/job.local.proxy"
    unset X509_RUN_AS_SERVER
    $ARC_LOCATION/libexec/downloader -p -c 'local' "$RUNTIME_CONTROL_DIR" "$RUNTIME_JOB_DIR" 2>>${RUNTIME_CONTROL_DIR$
    if [ $? -ne '0' ] ; then
      echo 'ERROR: Downloader failed.' 1>&2
      RESULT=1
    fi
EOSCR
  fi
}

##############################################################
# Add std... to job arguments
##############################################################
include_std_streams () {
  if [ ! -z "$joboption_stdin" ] ; then
    joboption_args="$joboption_args <\$RUNTIME_JOB_STDIN"
  fi
  if [ ! -z "$joboption_stdout" ] ; then
    joboption_args="$joboption_args 1>\$RUNTIME_JOB_STDOUT"
  fi
  if [ ! -z "$joboption_stderr" ] ; then
    if [ X"$joboption_stderr" = X"$joboption_stdout" ] ; then
      joboption_args="$joboption_args 2>&1"
    else
      joboption_args="$joboption_args 2>\$RUNTIME_JOB_STDERR"
    fi
  fi
}

##############################################################
# Runtime configuration
##############################################################
configure_runtime () {
  i=0
  eval "var_is_set=\${joboption_runtime_$i+yes}"
  while [ ! -z "${var_is_set}" ] ; do
    if [ "$i" = '0' ] ; then
      echo "if [ ! -z \"\$RUNTIME_CONFIG_DIR\" ] ; then" >> $LRMS_JOB_SCRIPT
    fi
    eval "var_value=\"\${joboption_runtime_$i}\""
    echo "  if [ -r \"\${RUNTIME_CONFIG_DIR}/${var_value}\" ] ; then" >> $LRMS_JOB_SCRIPT
    echo "    source \${RUNTIME_CONFIG_DIR}/${var_value} 2 " >> $LRMS_JOB_SCRIPT
    echo "  fi" >> $LRMS_JOB_SCRIPT
    i=$(( i + 1 ))
    eval "var_is_set=\${joboption_runtime_$i+yes}"
  done
  if [ ! "$i" = '0' ] ; then
    echo "fi" >> $LRMS_JOB_SCRIPT
  fi
  echo "" >> $LRMS_JOB_SCRIPT
}
##############################################################
# move files to node
##############################################################
move_files_to_node () {
  echo "RUNTIME_LOCAL_SCRATCH_DIR=\${RUNTIME_LOCAL_SCRATCH_DIR:-$RUNTIME_LOCAL_SCRATCH_DIR}" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_FRONTEND_SEES_NODE=\${RUNTIME_FRONTEND_SEES_NODE:-$RUNTIME_FRONTEND_SEES_NODE}" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_NODE_SEES_FRONTEND=\${RUNTIME_NODE_SEES_FRONTEND:-$RUNTIME_NODE_SEES_FRONTEND}" >> $LRMS_JOB_SCRIPT
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
  if [ ! -z "$RUNTIME_LOCAL_SCRATCH_DIR" ] ; then
    # moving (!!!!! race condition here - while there is no job directory
    # gridftp can create the one with the same name !!!!!)
    RUNTIME_NODE_JOB_DIR="$RUNTIME_LOCAL_SCRATCH_DIR"/`basename "$RUNTIME_JOB_DIR"`
    rm -rf "$RUNTIME_NODE_JOB_DIR"
    mkdir -p "$RUNTIME_NODE_JOB_DIR"
    mv "$RUNTIME_JOB_DIR" "$RUNTIME_LOCAL_SCRATCH_DIR"
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
      if [ ! -z "$RUNTIME_JOB_STDOUT__" ] ; then
        ln -s "$RUNTIME_JOB_STDOUT" "$RUNTIME_JOB_STDOUT__"
      fi
      if [ "$RUNTIME_JOB_STDOUT__" != "$RUNTIME_JOB_STDERR__" ] ; then
        if [ ! -z "$RUNTIME_JOB_STDOUT__" ] ; then
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
EOSCR
}

##############################################################
# move files back to frontend
##############################################################
move_files_to_frontend () {
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
  if [ ! -z "$RUNTIME_LOCAL_SCRATCH_DIR" ] ; then
    if [ "$RUNTIME_FRONTEND_SEES_NODE" = "no" ] ; then
      # just move it
      rm -rf "$RUNTIME_FRONTEND_JOB_DIR"
      mv "$RUNTIME_NODE_JOB_DIR" `dirname "$RUNTIME_FRONTEND_JOB_DIR"`
    else
      # remove links
      rm -f "$RUNTIME_JOB_STDOUT" 2>/dev/null
      rm -f "$RUNTIME_JOB_STDERR" 2>/dev/null
      # move whole directory
      cp -pR "$RUNTIME_NODE_JOB_DIR" `dirname "$RUNTIME_FRONTEND_JOB_DIR"`
      rm -rf "$RUNTIME_NODE_JOB_DIR"
    fi
  fi
  echo "exitcode=$RESULT" >> "$RUNTIME_JOB_DIAG"
  exit $RESULT
EOSCR
}

##############################################################
# copy runtime settings to jobscript
##############################################################
setup_runtime_env () {
  echo "RUNTIME_JOB_DIR=$joboption_directory" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_JOB_STDIN=$joboption_stdin" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_JOB_STDOUT=$joboption_stdout" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_JOB_STDERR=$joboption_stderr" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_JOB_DIAG=${joboption_directory}.diag" >> $LRMS_JOB_SCRIPT
  # Adjust working directory for tweaky nodes
  # RUNTIME_GRIDAREA_DIR should be defined by external means on nodes
  echo "if [ ! -z \"\$RUNTIME_GRIDAREA_DIR\" ] ; then" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_DIR=\$RUNTIME_GRIDAREA_DIR/\`basename \$RUNTIME_JOB_DIR\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_STDIN=\`echo \"\$RUNTIME_JOB_STDIN\" | sed \"s#^$joboption_directory#\$RUNTIME_JOB_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_STDOUT=\`echo \"\$RUNTIME_JOB_STDOUT\" | sed \"s#^$joboption_directory#\$RUNTIME_JOB_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_STDERR=\`echo \"\$RUNTIME_JOB_STDERR\" | sed \"s#^$joboption_directory#\$RUNTIME_JOB_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_DIAG=\`echo \"\$RUNTIME_JOB_DIAG\" | sed \"s#^$joboption_directory#\$RUNTIME_JOB_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_CONTROL_DIR=\`echo \"\$RUNTIME_CONTROL_DIR\" | sed \"s#^$joboption_directory#\$RUNTIME_JOB_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "fi" >> $LRMS_JOB_SCRIPT
}


##############################################################
# change to runtime dir and setup timed run
##############################################################
cd_and_run () {
  echo "# Changing to session directory" >> $LRMS_JOB_SCRIPT
  echo "cd \$RUNTIME_JOB_DIR" >> $LRMS_JOB_SCRIPT
  echo "export HOME=\$RUNTIME_JOB_DIR" >> $LRMS_JOB_SCRIPT

  if [ ! -z "$NODENAME" ] ; then
    echo "nodename=\`$NODENAME\`" >> $LRMS_JOB_SCRIPT
    echo "echo \"nodename=\$nodename\" >> \"\$RUNTIME_JOB_DIAG\"" >> $LRMS_JOB_SCRIPT
  fi

  if [ -z "$GNU_TIME" ] ; then
    echo "$joboption_args" >> $LRMS_JOB_SCRIPT
  else
    echo "$GNU_TIME -o \"\$RUNTIME_JOB_DIAG\" -a -f '\
WallTime=%es\nKernelTime=%Ss\nUserTime=%Us\nCPUUsage=%P\n\
MaxResidentMemory=%MkB\nAverageResidentMemory=%tkB\n\
AverageTotalMemory=%KkB\nAverageUnsharedMemory=%DkB\n\
AverageUnsharedStack=%pkB\nAverageSharedMemory=%XkB\n\
PageSize=%ZB\nMajorPageFaults=%F\nMinorPageFaults=%R\n\
Swaps=%W\nForcedSwitches=%c\nWaitSwitches=%w\n\
Inputs=%I\nOutputs=%O\nSocketReceived=%r\nSocketSent=%s\n\
Signals=%k\n' \
$joboption_args" >> $LRMS_JOB_SCRIPT
  fi
  echo "RESULT=\$?" >> $LRMS_JOB_SCRIPT
  echo "fi" >> $LRMS_JOB_SCRIPT
}

