#!/bin/bash
#
# TODO: Support for multiple control directories.
# TODO: Support for switching UID.
# TODO: Support for updating diag file during "scanning".

usage="$0 <scan_script> <unit_test>"


if test $# != 2; then
  echo "Two arguments are needed."
  echo $usage
  exit 1
fi

function goToParentAndRemoveDir() {
  cd ..
  rm -rf ${1}
}

# Clear time zone - important when converting start and end time of jobs to be written to diag file.
export TZ=

scan_script=$1
unit_test=$2
unit_test_basename=`basename ${unit_test}`
orig_srcdir=${SRCDIR}
ORIG_PATH=${PATH}

# Source unit test which should define a number of test functions having prefix 'test_'
. ${unit_test}

exitCode=0
errorOutput=""

for test in ${TESTS}; do
  testdir="scan_${unit_test_basename}_test_${test}"
  rm -rf ${testdir}
  mkdir -p ${testdir} ${testdir}/controldir/processing
  cd ${testdir}
  
  SRCDIR=../${orig_srcdir}
  
  export PATH=${ORIG_PATH}
  
  unset arc_test_configuration
  unset simulate_cmds
  unset simulator_output
  unset test_jobs
  unset input_diag_file
  unset expected_diag_file
  unset expected_kicked_jobs

  echo -n "."
  
  # Run test function
  test_${test}
  
  # Write ARC configuration (arc.conf)
  touch ${test}.arc.conf
  if test "x${arc_test_configuration}" != "x"; then
    echo "${arc_test_configuration}" > ${test}.arc.conf
  fi
  
  # Add sessiondir and controldir attributes to arc.conf configuration.
  if test $(grep '^[[]arex][[:space:]]*$' ${test}.arc.conf -c) -ge 1; then
    sed -i "/^[[]arex][[:space:]]*$/ a\
sessiondir=`pwd`\n\
controldir=`pwd`/controldir" ${test}.arc.conf
  else 
    echo $'\n'"[arex]"$'\n'"sessiondir=`pwd`"$'\n'"controldir=`pwd`/controldir" >> ${test}.arc.conf
  fi

  # Setup command simulation
  mkdir bin
  export PATH=$(pwd)/bin:${PATH}
  for cmd in sleep ${simulate_cmds}; do
    ln -s $(pwd)/../command-simulator.sh bin/${cmd}
  done
  if test ! -z "${simulator_output}"; then
    echo "${simulator_output}" > simulator_output
    export SIMULATOR_OUTCOME_FILE=$(pwd)/simulator_output
    export SIMULATOR_ERRORS_FILE=$(pwd)/simulator_errors
  fi
  
  # Create session directory and .status, .local, and .diag files.
  oIFS="${IFS}"
  IFS=$'\n'
  test_input_diag_list=( ${test_input_diag_list} )
  test_jobs=( ${test_jobs} )
  for i in $(seq 0 $(( ${#test_jobs[@]} - 1 )) ); do
    IFS=" " job=( ${test_jobs[$i]} )
    mkdir ${test}-${job[0]} # Create session directory
    echo "${job[1]}" > $(pwd)/controldir/processing/job.${job[0]}.status
    echo "localid=${job[0]}" >  $(pwd)/controldir/job.${job[0]}.local
    echo "sessiondir=$(pwd)/${test}-${job[0]}"  >> $(pwd)/controldir/job.${job[0]}.local

    IFS=";" job_diag=( ${test_input_diag_list[$i]} )
    printf "%s\n" "${job_diag[@]}" >  $(pwd)/${test}-${job[0]}.diag
  done <<< "${test_jobs}"
  IFS="${oIFS}"


  # Execute scan script
  script_output=$(../${scan_script} --config $(pwd)/${test}.arc.conf $(pwd)/controldir 2>&1)
  if test $? -ne 0; then
    echo -n "F"
    errorOutput="$errorOutput"$'\n\n'"Error: Scan script \"${scan_script}\" failed:"$'\n'"${script_output}"
    exitCode=$((exitCode + 1))
    goToParentAndRemoveDir ${testdir}
    continue
  fi


  # Check if commands were executed in expected order, with expected arguments
  if test ! -z "${simulator_output}"; then
    if test -f ${SIMULATOR_ERRORS_FILE}; then
      echo -n "F"
      errorOutput="$errorOutput"$'\n\n'"Error: Scan script \"${scan_script}\" failed:"$'\n'"Wrong command executed, or wrong arguments passed:"$'\n'"$(cat ${SIMULATOR_ERRORS_FILE})"
      exitCode=$((exitCode + 1))
      goToParentAndRemoveDir ${testdir}
      continue
    fi
  fi

  # TODO: Check Errors file
  
  # Diagnostic file (.diag)
  # TODO: Maybe check for possible .diag files not specified in 'expected_diags' list.
  for expected_diag in ${expected_diags}; do
    oIFS="${IFS}"
    IFS=";" expected_diag=( $expected_diag )
    IFS="${oIFS}"
    # Check if diag file exist
    diag_file=${test}-${expected_diag[0]}.diag
    
    printf "%s\n" "${expected_diag[@]:1}" | sort -u -t= -k1,1 > expected-${diag_file}
    # TODO: Filtering is too agressive. Some attributes appears multiple time (e.g. 'nodelist' attribute).
    tac ${diag_file}  | grep -v "^\(#\|[[:space:]]*$\)" | sort -u -t= -k1,1 > filtered-${diag_file}
    diag_diff=$(diff -u expected-${diag_file} filtered-${diag_file})
    if test $? != 0; then
      echo -n "F"
      exitCode=$((exitCode + 1))
      errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"Diag for job (${expected_diag[0]}) differs from expected:"$'\n'"${diag_diff}"
      goToParentAndRemoveDir ${testdir}
      continue 2
    fi
  done
 
  # Check exit codes (.lrms_done)
  # TODO: Maybe check for possible .lrms_done files not specified in 'expected_exit_codes' list.
  for expected_exit_code in ${expected_exit_codes}; do
    oIFS="${IFS}"
    IFS=";" expected_exit_code=( $expected_exit_code )
    IFS="${oIFS}"
    read job_exit_code < controldir/job.${expected_exit_code[0]}.lrms_done
    if test "${job_exit_code%% *}" != "${expected_exit_code[1]}"; then
      echo -n "F"
      exitCode=$((exitCode + 1))
      errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"Exit code for job (${expected_exit_code[0]}) was expected to be '${expected_exit_code[1]}', but '${job_exit_code%% *}' was found."
      goToParentAndRemoveDir ${testdir}
      continue 2
    fi
  done
  
  # Check kicked jobs
  if test ! -z "${expected_kicked_jobs}" && test ! -f gm_kick_test_file; then
    echo -n "F"
    exitCode=$((exitCode + 1))
    errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"No jobs kicked, however the following jobs were expected to be kicked:"$'\n'"${expected_kicked_jobs}"
    goToParentAndRemoveDir ${testdir}
    continue
  fi
  jobs_not_kicked=""
  for job in ${expected_kicked_jobs}; do
    if test $(grep -c "^${job}\$" gm_kick_test_file) == 0; then
      jobs_not_kicked="${jobs_not_kicked}"$'\n'"${job}"
      continue
    fi
    # Remove first occurance of job in kick list.
    sed -i "0,/^${job}\$/ {/^${job}\$/ d}" gm_kick_test_file
  done
  if test ! -z "${jobs_not_kicked}"; then
    echo -n "F"
    exitCode=$((exitCode + 1))
    errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"The following jobs was expected to be kicked, but was not:${jobs_not_kicked}"
    goToParentAndRemoveDir ${testdir}
    continue
  fi

  # Check if there are any entries left in kick file.
  if test "$(wc -w gm_kick_test_file | cut -f 1 -d ' ')" -gt 0; then
    echo -n "F"
    exitCode=$((exitCode + 1))
    errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"The following jobs was unexpectedly kicked:"$'\n'"$(sed 's#^#  #' gm_kick_test_file)"
    goToParentAndRemoveDir ${testdir}
    continue
  fi
 
  # Make post check if test_post_check function is defined.
  if test "x$(type -t test_post_check)" == "xfunction"; then
    post_check_output=$(test_post_check ${lrms_script_name})
    if test $? != 0; then
      echo -n "F"
      exitCode=$((exitCode + 1))
      errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"${post_check_output}"
      goToParentAndRemoveDir ${testdir}
      continue
    fi
  fi

  goToParentAndRemoveDir ${testdir}
done

if test ${exitCode} = 0; then
  echo $'\n\n'"OK ("$(echo ${TESTS} | wc -w)")"
else
  echo "${errorOutput}"$'\n'
fi

exit ${exitCode}
