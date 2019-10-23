######################################################
#Common functions for submit scripts
######################################################

# This script should not be executed directly but is sourced in
# from various backend scripts that itself are called from the
# grid manager. Its purpose is to prepare the runtime environments,
# which is almost the same procedure invariant of the backend
# used.

sourcewithargs () {
  script=$1
  shift
  . $script
}

#
# Exits with 0 if the argument is all digits
#
is_number () {
    /usr/bin/perl -e 'exit 1 if $ARGV[0] !~ m/^\d+$/' "$1"
}

#
# Initial parsing and environemtn setub for submission scripts
# THIS FUNCTION USES FUNCTIONS DEFINED IN LRMS_common.sh
#
common_init () {
    # parse grami file
    parse_grami_file $GRAMI_FILE
    # parse configuration
    parse_arc_conf
    # read pbs-specific environment
    . ${pkgdatadir}/configure-${joboption_lrms}-env.sh || exit $?
    # init common LRMS environmental variables
    init_lrms_env
}

# defines failures_file
define_failures_file () {
    failures_file="$joboption_controldir/job.$joboption_gridid.failed"
}


# checks any scratch is defined (shared or local)
check_any_scratch () {
    if [ -z "${RUNTIME_NODE_SEES_FRONTEND}" ] ; then
        if [ -z "${RUNTIME_LOCAL_SCRATCH_DIR}" ] ; then
            echo "Need to know at which directory to run job: RUNTIME_LOCAL_SCRATCH_DIR must be set if RUNTIME_NODE_SEES_FRONTEND is empty" 1>&2
            echo "Submission: Configuration error.">>"$failures_file"
            exit 1
        fi
    fi
}

#
# Sets a default memory limit for jobs that don't have one
#
set_req_mem () {
    if ! is_number "$joboption_memory"; then

        echo "---------------------------------------------------------------------" 1>&2
        echo "WARNING: The job description contains no explicit memory requirement." 1>&2

        if is_number "$CONFIG_defaultmemory"; then
            joboption_memory=$CONFIG_defaultmemory

            echo "         A default memory limit taken from 'defaultmemory' in        " 1>&2
            echo "         arc.conf will apply.                                        " 1>&2
            echo "         Limit is: $CONFIG_defaultmemory MB.                         " 1>&2
        else
            echo "         No 'defaultmemory' enforcement in in arc.conf.              " 1>&2
            echo "         JOB WILL BE PASSED TO BATCH SYSTEM WITHOUT MEMORY LIMIT !!! " 1>&2
        fi
        echo "---------------------------------------------------------------------" 1>&2
    fi
}

set_count () {
  if [ -z "$joboption_count" ] || [ "$joboption_count" -le 1 ] ; then
    joboption_count=1
    joboption_countpernode=-1
    joboption_numnodes=-1
  fi
}

##############################################################
# create temp job script
##############################################################

mktempscript () {
   # File name to be used for temporary job script
   LRMS_JOB_SCRIPT=`mktemp ${TMPDIR}/${joboption_lrms}_job_script.XXXXXX`
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
      echo "Something is wrong. Either somebody is deleting files or I cannot write to ${TMPDIR}"
      rm -f "$LRMS_JOB_SCRIPT" "$LRMS_JOB_OUT" "$LRMS_JOB_ERR"
      exit 1
   fi
}

##############################################################
# Jobscript resource usage accounting
##############################################################

accounting_init () {
    cat >> $LRMS_JOB_SCRIPT <<EOSCR
echo "Detecting resource accounting method available for the job." 1>&2
JOB_ACCOUNTING=""
# Try to use cgroups first
if command -v arc-job-cgroup >/dev/null 2>&1; then
    echo "Found arc-job-cgroup tool: trying to initialize accounting cgroups for the job." 1>&2
    while true; do
        # memory cgroup
        memory_cgroup="\$( arc-job-cgroup -m -n $joboption_gridid )"
        if [ \$? -ne 0 -o -z "\$memory_cgroup" ]; then
            echo "Failed to initialize memory cgroup for accounting." 1>&2
            break;
        fi
        # cpuacct cgroup
        cpuacct_cgroup="\$( arc-job-cgroup -c -n $joboption_gridid )"
        if [ \$? -ne 0 -o -z "\$cpuacct_cgroup" ]; then
            echo "Failed to initialize cpuacct cgroup for accounting." 1>&2
            break;
        fi
        echo "Using cgroups method for job accounting" 1>&2
        JOB_ACCOUNTING="cgroup"
        break;
    done
fi

# Fallback to GNU_TIME if cgroups are not working
if [ -z "\$JOB_ACCOUNTING" ]; then
    GNU_TIME='$GNU_TIME'
    echo "Looking for \$GNU_TIME tool for accounting measurements" 1>&2
    if [ ! -z "\$GNU_TIME" ] && ! "\$GNU_TIME" --version >/dev/null 2>&1; then
        echo "GNU time is not found at: \$GNU_TIME" 1>&2
    else
        echo "GNU time found and will be used for job accounting." 1>&2
        JOB_ACCOUNTING="gnutime"
    fi
fi

# Nothing works: rely on LRMS only
if [ -z "\$JOB_ACCOUNTING" ]; then
    echo "Failed to use both cgroups and GNU time for resource usage accounting. Accounting relies on LRMS information only." 1>&2
fi

EOSCR
if [ -n "$ACCOUNTING_WN_INSTANCE" ]; then
  echo "# Define accounting WN instance tag" >> $LRMS_JOB_SCRIPT
  echo "ACCOUNTING_WN_INSTANCE='$ACCOUNTING_WN_INSTANCE'" >> $LRMS_JOB_SCRIPT
fi
}

accounting_end () {
    cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
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
EOSCR
}

detect_wn_systemsoftware () {
cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
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

EOSCR
}

##############################################################
# Add environment variables
##############################################################

add_user_env () {
   echo "# Setting environment variables as specified by user" >> $LRMS_JOB_SCRIPT
   has_gridglobalid=''
   i=0
   eval "var_is_set=\${joboption_env_$i+yes}"
   while [ ! -z "${var_is_set}" ] ; do
      eval "var_value=\${joboption_env_$i}"
      if [ "$var_value" ] && [ -z "${var_value##GRID_GLOBAL_JOBID=*}" ]; then
          has_gridglobalid=yes
      fi
      var_escaped=`echo "$var_value" | sed "s/'/'\\\\\''/g"`
      echo "export '${var_escaped}'" >> $LRMS_JOB_SCRIPT
      i=$(( $i + 1 ))
      eval "var_is_set=\${joboption_env_$i+yes}"
   done

   # guess globalid in case not already provided
   if [ -z "$has_gridglobalid" ]; then
      hostname=`/usr/bin/perl -MSys::Hostname -we 'print hostname'`
      hostname=${CONFIG_hostname:-$hostname}
      # not configurable any longer
      gm_port=2811
      gm_mount_point="/jobs"
      echo "export GRID_GLOBAL_JOBID='gsiftp://$hostname:$gm_port$gm_mount_point/$joboption_gridid'" >> $LRMS_JOB_SCRIPT
   fi

   echo "" >> $LRMS_JOB_SCRIPT
}

sourcewithargs_jobscript () {
   echo "# source with arguments for DASH shells" >> $LRMS_JOB_SCRIPT
   echo "sourcewithargs() {" >> $LRMS_JOB_SCRIPT
   echo "script=\$1" >> $LRMS_JOB_SCRIPT
   echo "shift" >> $LRMS_JOB_SCRIPT
   echo ". \$script" >> $LRMS_JOB_SCRIPT
   echo "}" >> $LRMS_JOB_SCRIPT
}

##############################################################
#  RunTimeEnvironemt Functions
##############################################################
RTE_include_default () {
    default_rte_dir="${CONFIG_controldir}/rte/default/"
    if [ -d "$default_rte_dir" ]; then
        # get default RTEs
        default_rtes=` find "$default_rte_dir" ! -type d -exec test -e {} \; -print | sed "s#^$default_rte_dir##" `
        if [ -n "$default_rtes" ]; then
            # Find last RTE index defined
            rte_idx=0
            eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
            while [ -n "${is_rte}" ] ; do
                rte_idx=$(( rte_idx + 1 ))
                eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
            done
            req_idx=$rte_idx
            # Add default RTEs to the list
            for rte_name in $default_rtes; do
                # check if already included into the list of requested RTEs
                check_idx=0
                while [ $check_idx -lt $req_idx ]; do
                    eval "check_rte=\${joboption_runtime_${check_idx}}"
                    if [ "$rte_name" = "$check_rte" ]; then
                        echo "$rte_name RTE is already requested: skipping the same default RTE injection." 1>&2
                        continue 2
                    fi
                    check_idx=$(( check_idx + 1 ))
                done
                eval "joboption_runtime_${rte_idx}=$rte_name"
                rte_idx=$(( rte_idx + 1 ))
            done
        fi
    fi
    unset default_rte_dir default_rtes is_rte rte_idx rte_name req_idx check_idx check_rte
}

RTE_path_set () {
    rte_params_path="${CONFIG_controldir}/rte/params/${rte_name}"
    if [ ! -e "$rte_params_path" ]; then
        rte_params_path=""
    fi
    rte_path="${CONFIG_controldir}/rte/enabled/${rte_name}"
    if [ ! -e "$rte_path" ]; then
        rte_path="${CONFIG_controldir}/rte/default/${rte_name}"
        if [ ! -e "$rte_path" ]; then
            echo "ERROR: Requested RunTimeEnvironment ${rte_name} is missing, broken or not enabled." 1>&2
            exit 1
        fi
    fi
    # check RTE is empty
    unset rte_empty
    [ -s "$rte_path" ] || rte_empty=1
}

RTE_add_optional_args () {
    # for RTE defined by 'rte_idx' adds optional arguments to 'args_value' if any
    arg_idx=1
    eval "is_arg=\${joboption_runtime_${rte_idx}_${arg_idx}+yes}"
    while [ -n "${is_arg}" ] ; do
        eval "arg_value=\${joboption_runtime_${rte_idx}_${arg_idx}}"
        # Use printf in order to handle backslashes correctly (bash vs dash)
        arg_value=` printf "%s" "$arg_value" | sed 's/"/\\\\\\\"/g' `
        args_value="$args_value \"${arg_value}\""
        arg_idx=$(( arg_idx + 1 ))
        eval "is_arg=\${joboption_runtime_${rte_idx}_${arg_idx}+yes}"
    done
    unset arg_idx arg_value is_arg 
}

RTE_to_jobscript () {
    rte_idx=0
    eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    while [ -n "${is_rte}" ] ; do
        eval "rte_name=\"\${joboption_runtime_${rte_idx}}\""
        # define rte_path
        RTE_path_set
        # skip empty RTEs
        if [ -z "$rte_empty" ]; then
            # add RTE script content as a function into the job script
            echo "# RunTimeEnvironment function for ${rte_name}:" >> $LRMS_JOB_SCRIPT
            echo "RTE_function_${rte_idx} () {" >> $LRMS_JOB_SCRIPT
            [ -n "${rte_params_path}" ] && cat "${rte_params_path}" >> $LRMS_JOB_SCRIPT
            cat "${rte_path}" >> $LRMS_JOB_SCRIPT
            echo "}" >> $LRMS_JOB_SCRIPT
        else
            # mark RTE as empty to skip further processing
            eval "joboption_runtime_${rte_idx}_empty=1"
        fi
        # next RTE
        rte_idx=$(( rte_idx + 1 ))
        eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    done
    unset is_rte rte_idx rte_name rte_path rte_empty
}

RTE_jobscript_call () {
    rte_stage=$1
    if [ "$rte_stage" = '1' ]; then
        RTE_to_jobscript
    fi
    echo "# Running RTE scripts (stage ${rte_stage})" >> $LRMS_JOB_SCRIPT
    echo "runtimeenvironments=" >> $LRMS_JOB_SCRIPT

    rte_idx=0
    eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    while [ -n "${is_rte}" ] ; do
        # RTE name is for admin-friendly logging only
        eval "rte_name=\"\${joboption_runtime_${rte_idx}}\""
        echo "runtimeenvironments=\"\${runtimeenvironments}${rte_name};\"" >> $LRMS_JOB_SCRIPT
        # add call if RTE is not empty
        eval "is_empty=\${joboption_runtime_${rte_idx}_empty+yes}"
        if [ -z "${is_empty}" ]; then 
            # compose arguments value for RTE function call
            args_value="${rte_stage} "
            RTE_add_optional_args
            # add function call to job script
            echo "# Calling ${rte_name} function: " >> $LRMS_JOB_SCRIPT
            # Use printf in order to handle backslashes correctly (bash vs dash)
            printf "RTE_function_${rte_idx} ${args_value}\n" >> $LRMS_JOB_SCRIPT
            echo "if [ \$? -ne 0 ]; then" >> $LRMS_JOB_SCRIPT
            echo "    echo \"Runtime ${rte_name} stage ${rte_stage} execution failed.\" 1>&2" >> $LRMS_JOB_SCRIPT
            echo "    echo \"Runtime ${rte_name} stage ${rte_stage} execution failed.\" 1>\"\${RUNTIME_JOB_DIAG}\"" >> $LRMS_JOB_SCRIPT
            echo "    exit 1" >> $LRMS_JOB_SCRIPT
            echo "fi" >> $LRMS_JOB_SCRIPT
            echo "" >> $LRMS_JOB_SCRIPT
        fi
        rte_idx=$(( rte_idx + 1 ))
        eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    done
    unset rte_idx is_rte is_empty rte_name args_value rte_stage
}

RTE_stage0 () {
    RTE_include_default
    rte_idx=0
    eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    while [ -n "${is_rte}" ] ; do
        eval "rte_name=\"\${joboption_runtime_${rte_idx}}\""
        # define rte_path
        RTE_path_set
        # skip empty RTEs
        if [ -z "$rte_empty" ]; then
            # define arguments
            args_value="0 "
            RTE_add_optional_args
            # run RTE stage 0
            # WARNING!!! IN SOME CASES DUE TO DIRECT SOURCING OF RTE SCRIPT WITHOUT ANY SAFETY CHECKS 
            #            SPECIALLY CRAFTED RTES CAN BROKE CORRECT SUBMISSION (e.g. RTE redefine 'rte_idx' variable)
            [ -n "${rte_params_path}" ] && source "${rte_params_path}" 1>&2
            sourcewithargs "$rte_path" $args_value 1>&2
            rte0_exitcode=$?
            if [ $rte0_exitcode -ne 0 ] ; then
                echo "ERROR: Runtime script ${rte_name} stage 0 execution failed with exit code ${rte0_exitcode}" 1>&2
                exit 1
            fi
        fi
        rte_idx=$(( rte_idx + 1 ))
        eval "is_rte=\${joboption_runtime_${rte_idx}+yes}"
    done

    # joboption_count might have been changed by an RTE. Save it for accounting purposes.
    if [ -n "$joboption_count" ]; then
        diagfile="${joboption_controldir}/job.${joboption_gridid}.diag"
        echo "Processors=$joboption_count" >> "$diagfile"
        if [ -n "$joboption_numnodes" ]; then
            echo "Nodecount=$joboption_numnodes" >> "$diagfile"
        fi
    fi
    unset rte_idx is_rte rte_name rte_path rte_empty rte0_exitcode
}

RTE_stage1 () {
    RTE_jobscript_call 1
}

RTE_stage2 () {
    RTE_jobscript_call 2
}

##############################################################
# Add std... to job arguments
##############################################################
include_std_streams () {
  input_redirect=
  output_redirect=
  if [ ! -z "$joboption_stdin" ] ; then
    input_redirect="<\$RUNTIME_JOB_STDIN"
  fi
  if [ ! -z "$joboption_stdout" ] ; then
    output_redirect="1>\$RUNTIME_JOB_STDOUT"
  fi
  if [ ! -z "$joboption_stderr" ] ; then
    if [ "$joboption_stderr" = "$joboption_stdout" ] ; then
      output_redirect="$output_redirect 2>&1"
    else
      output_redirect="$output_redirect 2>\$RUNTIME_JOB_STDERR"
    fi
  fi
}

##############################################################
# move files to node
##############################################################
move_files_to_node () {
  if [ "$joboption_count" -eq 1 ] || [ ! -z "$RUNTIME_ENABLE_MULTICORE_SCRATCH" ] || [ "$joboption_count" -eq "$joboption_countpernode" ]; then
    echo "RUNTIME_LOCAL_SCRATCH_DIR=\${RUNTIME_LOCAL_SCRATCH_DIR:-$RUNTIME_LOCAL_SCRATCH_DIR}" >> $LRMS_JOB_SCRIPT
  else
    echo "RUNTIME_LOCAL_SCRATCH_DIR=\${RUNTIME_LOCAL_SCRATCH_DIR:-}" >> $LRMS_JOB_SCRIPT
  fi
  echo "RUNTIME_FRONTEND_SEES_NODE=\${RUNTIME_FRONTEND_SEES_NODE:-$RUNTIME_FRONTEND_SEES_NODE}" >> $LRMS_JOB_SCRIPT
  echo "RUNTIME_NODE_SEES_FRONTEND=\${RUNTIME_NODE_SEES_FRONTEND:-$RUNTIME_NODE_SEES_FRONTEND}" >> $LRMS_JOB_SCRIPT
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
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
EOSCR
}


##############################################################
# Clean up output files in the local scratch dir
##############################################################
clean_local_scratch_dir_output () {
  # "moveup" parameter will trigger output files moving to one level up
  if [ "x$1" = "xmoveup" ]; then
	  move_files_up=1
  fi
  # Calculate the scratch size at the end of execution
  echo '# Measuring used scratch space' >> $LRMS_JOB_SCRIPT
  echo 'echo "usedscratch=$( du -sb "$RUNTIME_JOB_DIR" | sed "s/\s.*$//" )" >> "$RUNTIME_JOB_DIAG"' >> $LRMS_JOB_SCRIPT
  # There is no sense to keep trash till GM runs uploader
  echo '# Cleaning up extra files in the local scratch' >> $LRMS_JOB_SCRIPT
  echo 'if [ ! -z  "$RUNTIME_LOCAL_SCRATCH_DIR" ] ; then' >> $LRMS_JOB_SCRIPT
  # Delete all files except listed in job.#.output
  echo '  find ./ -type l -exec rm -f "{}" ";"' >> $LRMS_JOB_SCRIPT
  echo '  chmod -R u+w "./"' >> $LRMS_JOB_SCRIPT
  
  if [ -f "$joboption_controldir/job.$joboption_gridid.output" ] ; then
    cat "$joboption_controldir/job.$joboption_gridid.output" | \
    # remove leading backslashes, if any
    sed 's/^\/*//' | \
    # backslashes and spaces are escaped with a backslash in job.*.output. The
    # shell built-in read undoes this escaping.
    while read name rest; do
  
      # make it safe for shell by replacing single quotes with '\''
      name=`printf "%s" "$name"|sed "s/'/'\\\\\\''/g"`;
  
      # protect from deleting output files including those in the dynamic list
      if [ "${name#@}" != "$name" ]; then     # Does $name start with a @ ?
  
        dynlist=${name#@}
        echo "  dynlist='$dynlist'" >> $LRMS_JOB_SCRIPT
        cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
  chmod -R u-w "./$dynlist" 2>/dev/null
  cat "./$dynlist" | while read name rest; do
    chmod -R u-w "./$name" 2>/dev/null
  done
EOSCR
      else
        printf "%s\n" "  chmod -R u-w \"\$RUNTIME_JOB_DIR\"/'$name' 2>/dev/null" >> $LRMS_JOB_SCRIPT
        if [ -n "$move_files_up" -a -z "${RUNTIME_NODE_SEES_FRONTEND}" ] ; then
           printf "%s\n" "  mv \"\$RUNTIME_JOB_DIR\"/'$name' ../." >> $LRMS_JOB_SCRIPT
        fi
      fi
    done
  fi
  
  echo '  find ./ -type f -perm /200 -exec rm -f "{}" ";"' >> $LRMS_JOB_SCRIPT
  echo '  chmod -R u+w "./"' >> $LRMS_JOB_SCRIPT
  echo 'fi' >> $LRMS_JOB_SCRIPT
  echo "" >> $LRMS_JOB_SCRIPT
}

##############################################################
# move files back to frontend
##############################################################
move_files_to_frontend () {
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
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
  echo "  RUNTIME_JOB_STDIN=\`echo \"\$RUNTIME_JOB_STDIN\" | sed \"s#^\$RUNTIME_JOB_DIR#\$RUNTIME_GRIDAREA_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_STDOUT=\`echo \"\$RUNTIME_JOB_STDOUT\" | sed \"s#^\$RUNTIME_JOB_DIR#\$RUNTIME_GRIDAREA_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_STDERR=\`echo \"\$RUNTIME_JOB_STDERR\" | sed \"s#^\$RUNTIME_JOB_DIR#\$RUNTIME_GRIDAREA_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_JOB_DIAG=\`echo \"\$RUNTIME_JOB_DIAG\" | sed \"s#^\$RUNTIME_JOB_DIR#\$RUNTIME_GRIDAREA_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "  RUNTIME_CONTROL_DIR=\`echo \"\$RUNTIME_CONTROL_DIR\" | sed \"s#^\$RUNTIME_JOB_DIR#\$RUNTIME_GRIDAREA_DIR#\"\`" >> $LRMS_JOB_SCRIPT
  echo "fi" >> $LRMS_JOB_SCRIPT
}


##############################################################
# change to runtime dir and setup timed run
##############################################################
cd_and_run () {

  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
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
EOSCR

  if [ ! -z "$NODENAME" ] ; then
    cat >> $LRMS_JOB_SCRIPT <<EOSCR
# Write nodename if not already written in LRMS-specific way
if [ -z "\$NODENAME_WRITTEN" ] ; then
  nodename=\`$NODENAME\`
  echo "nodename=\$nodename" >> "\$RUNTIME_JOB_DIAG"
fi
EOSCR
  fi

  #TODO this should probably be done on headnode instead
  echo "echo \"Processors=${joboption_count}\" >> \"\$RUNTIME_JOB_DIAG\"" >> $LRMS_JOB_SCRIPT

  # Define executable and check it exists on the worker node
  echo "executable='$joboption_arg_0'" >> $LRMS_JOB_SCRIPT
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
# Check if executable exists
if [ ! -f "$executable" ]; 
then 
  echo "Path \"$executable\" does not seem to exist" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1
fi
EOSCR

  # In case the job executable is written in a scripting language and the
  # interpreter is not found, the error message printed by GNU_TIME is
  # misleading.  This will print a more appropriate error message.
  cat >> $LRMS_JOB_SCRIPT <<'EOSCR'
# See if executable is a script, and extract the name of the interpreter
line1=`dd if="$executable" count=1 2>/dev/null | head -n 1`
shebang=`echo $line1 | sed -n 's/^#! *//p'`
interpreter=`echo $shebang | awk '{print $1}'`
if [ "$interpreter" = /usr/bin/env ]; then interpreter=`echo $shebang | awk '{print $2}'`; fi
# If it's a script and the interpreter is not found ...
[ "x$interpreter" = x ] || type "$interpreter" > /dev/null 2>&1 || {

  echo "Cannot run $executable: $interpreter: not found" 1>$RUNTIME_JOB_STDOUT 2>$RUNTIME_JOB_STDERR 1>&2
  exit 1; }
EOSCR
    # Check gnutime wrap is used for accounting
    cat >> $LRMS_JOB_SCRIPT <<EOSCR
if [ "x\$JOB_ACCOUNTING" = "xgnutime" ]; then
  \$GNU_TIME -o "\$RUNTIME_JOB_DIAG" -a -f '\
WallTime=%es\nKernelTime=%Ss\nUserTime=%Us\nCPUUsage=%P\n\
MaxResidentMemory=%MkB\nAverageResidentMemory=%tkB\n\
AverageTotalMemory=%KkB\nAverageUnsharedMemory=%DkB\n\
AverageUnsharedStack=%pkB\nAverageSharedMemory=%XkB\n\
PageSize=%ZB\nMajorPageFaults=%F\nMinorPageFaults=%R\n\
Swaps=%W\nForcedSwitches=%c\nWaitSwitches=%w\n\
Inputs=%I\nOutputs=%O\nSocketReceived=%r\nSocketSent=%s\n\
Signals=%k\n' \
$joboption_args $input_redirect $output_redirect
else
  $joboption_args $input_redirect $output_redirect
fi
RESULT=\$?

EOSCR
}

