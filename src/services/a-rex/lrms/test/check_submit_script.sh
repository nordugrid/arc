#!/bin/bash
#
# This script checks that submit scripts (passed as first argument) write the
# expected job options into lrms job script, with a job description as input.
# The expected job options and job description are defined in unit tests
# (passed as second argument).
#
# A LRMS job options unit test should define a number of functions prefixed
# 'test_', and a 'TESTS' variable listing these functions without the 'test_'
# prefix. These functions should define the 'job_description_input' and
# 'expected_job_options' environment variables. The former variable should
# contain a job description string in any language supported by ARC (e.g. xRSL),
# whereas the latter should contain the LRMS job options expected expected to be
# output by the LRMS job script based on the GRAMi file created from the job
# description input. Optionally the unit test can set the arc_test_configuration
# variable, which defines an ARC configuration (arc.conf), to be used when 
# executing the LRMS job script.
#

usage="$0 <submit_script> <unit_test>"

if test $# != 2; then
  echo "Two arguments are needed."
  echo $usage
  exit 1
fi

function goToParentAndRemoveDir() {
  export PATH=${ORIG_PATH}

  # Make the life of developers easy when modifications of unit-tests are needed:
  #   following code writes a new.diff files in case of unit-test had failed
  #   developer should check this diff against the changes made to batch backends
  #   if it is intented behaviour - replace the unit-test patch with a new diff
  if [ "x$TESTFAILED" = "x1" ]; then
    tmpdir=$( mktemp -d testdiff.XXXXXX )
    mkdir $tmpdir/a $tmpdir/b
    # Write basic LRMS job script with test substitutions
    sed -e "s#@TEST_JOB_ID@#${test}#g" \
        -e "s#@TEST_SESSION_DIR@#$(pwd)#g" \
        -e "s#@TEST_RUNTIME_CONFIG_DIR@#$(pwd)/rtes#" \
        -e "s#@TEST_ARC_LOCATION@#$(dirname $(pwd))#" \
        -e "s#@TEST_HOSTNAME@#$(uname -n)#" \
        "../basic-script.sh" > $tmpdir/a/basic-script.sh

    cp ${lrms_script_name} $tmpdir/b/basic-script.sh

    cd $tmpdir
    diff -ubBw ${test_ignore_matching_line} a/basic-script.sh b/basic-script.sh > $unit_test_basename-${test#test_}.new.diff
    cd - > /dev/null

    mv $tmpdir/$unit_test_basename-${test#test_}.new.diff ..
    rm -rf $tmpdir
  fi

  cd ..
  # do not remove unit test directory for failed test
  [ "x$TESTFAILED" = "x1" ] || rm -rf ${1}
  if test "${lrms_script_name}x" != "x"; then
    rm -f ${lrms_script_name} ${lrms_script_name}.out ${lrms_script_name}.err
  fi
}

submit_script=$1
unit_test=$2
unit_test_basename=$(basename ${unit_test})
orig_srcdir=${SRCDIR}
ORIG_PATH=${PATH}

# Source unit test which should define a number of test functions having prefix 'test_'
. ${unit_test}

exitCode=0
errorOutput=""

# Loop over all functions prefixed 'test_'
for test in ${TESTS}; do
  testdir="${unit_test_basename}_test_${test}"
  rm -rf ${testdir}
  mkdir -p ${testdir}/${test} ${testdir}/controldir
  cd ${testdir}
  testdir=$(pwd)
  
  SRCDIR=../${orig_srcdir}
  
  export PATH=${ORIG_PATH}

  unset arc_test_configuration
  unset job_description_input
  unset rtes
  unset ONLY_WRITE_JOBOPTIONS
  unset test_post_check
  unset test_ignore_matching_line
  
  echo -n "."
  
  # Run test function
  test_${test}
  
  if test "x${job_description_input}" = "x"; then
    echo -n "F"
    errorOutput="$errorOutput"$'\n\n'"Error: test_${test} in unit test \"${unit_test}\" doesn't define the 'job_description_input' environment variable."
    exitCode=$((exitCode + 1))
    goToParentAndRemoveDir ${testdir}
    continue
  fi
  
  if test ! -f "expected_lrms_job_script.tmpl"; then
    echo -n "F"
    errorOutput="$errorOutput"$'\n\n'"Error: test_${test} in unit test \"${unit_test}\" did not create the 'expected_lrms_job_script.tmpl' template file."
    exitCode=$((exitCode + 1))
    goToParentAndRemoveDir ${testdir}
    continue
  fi
  
  # Write ARC configuration (arc.conf)
  touch ${test}.arc.conf
  if test "x${arc_test_configuration}" != "x"; then
    echo "${arc_test_configuration}" > ${test}.arc.conf
  elif test "x${general_arc_test_configuration}" != "x"; then
    echo "${general_arc_test_configuration}" > ${test}.arc.conf
  fi
  sed -i "s#@PWD@#${testdir}#g" ${test}.arc.conf

  # Setup command simulation
  mkdir bin
  export PATH=$(pwd)/bin:${PATH}
  for cmd in ${simulate_cmds}; do
    ln -s $(pwd)/../command-simulator.sh bin/${cmd}
  done
  if test ! -z "${simulator_output}"; then
    echo "${simulator_output}" > simulator_output
    export SIMULATOR_OUTCOME_FILE=$(pwd)/simulator_output
    export SIMULATOR_ERRORS_FILE=$(pwd)/simulator_errors
  fi
  
  if test $(grep '^[[]arex][[:space:]]*$' ${test}.arc.conf -c) -ge 1; then
    sed -i "/^[[]arex][[:space:]]*$/ a\
sessiondir=$(pwd)\n\
controldir=$(pwd)/controldir" ${test}.arc.conf
  else 
    echo $'\n'"[arex]"$'\n'"sessiondir=$(pwd)"$'\n'"controldir=$(pwd)/controldir" >> ${test}.arc.conf
  fi


  # If defined, write RTEs to disk
  if test "x${rtes}" != "x"; then
    mkdir rtes
    mkdir -p "$(pwd)/controldir/rte/enabled"
    # Add runtimedir attribute to arc.conf. If 'arex' section does not exist, add it as well.
    if test $(grep '^[[]arex][[:space:]]*$' ${test}.arc.conf -c) -ge 1; then
      sed -i "/^[[]arex][[:space:]]*$/ a\
runtimedir=$(pwd)/rtes" ${test}.arc.conf
    else 
      echo $'\n'"[arex]"$'\n'"runtimedir=$(pwd)/rtes" >> ${test}.arc.conf
    fi
    for rte in ${rtes}; do
      echo "${!rte}" > rtes/${rte}
      chmod +x rtes/${rte}
      # 'enable' RTE
      ln -s "$(pwd)/rtes/${rte}" "$(pwd)/controldir/rte/enabled/${rte}"
    done
  fi

  # Write GRAMi file.
  ../${TEST_WRITE_GRAMI_FILE} --grami "${test}" --conf "${test}.arc.conf" "${job_description_input}" 2>&1 > /dev/null
  if test $? -ne 0; then
    echo -n "F"
    errorOutput="$errorOutput"$'\n\n'"Error: Writing GRAMI file failed."
    exitCode=$((exitCode + 1))
    goToParentAndRemoveDir ${testdir}
    continue
  fi
  
  # Execute submit script
  script_output=$(ONLY_WRITE_JOBSCRIPT="yes" ../${submit_script} --config $(pwd)/${test}.arc.conf $(pwd)/controldir/job.${test}.grami 2>&1)
  if test $? -ne 0; then
    echo -n "F"
    errorOutput="$errorOutput"$'\n\n'"Error: Submit script \"${submit_script}\" failed:"$'\n'"${script_output}"
    exitCode=$((exitCode + 1))
    goToParentAndRemoveDir ${testdir}
    continue
  fi
  lrms_script_name=$(echo "${script_output}" | grep "^Created file " | sed 's/Created file //')
  
  # Check if commands were executed in expected order, with expected arguments
  if test ! -z "${simulator_output}"; then
    if test -f "${SIMULATOR_ERRORS_FILE}"; then
      echo -n "F"
      errorOutput="$errorOutput"$'\n\n'"Error: Submit script \"${submit_script}\" failed:"$'\n'"Wrong command executed, or wrong arguments passed:"$'\n'"$(cat ${SIMULATOR_ERRORS_FILE})"
      exitCode=$((exitCode + 1))
      goToParentAndRemoveDir ${testdir}
      continue
    fi
  fi

  # Write expected LRMS job script to file.
  sed -e "s#@TEST_JOB_ID@#${test}#g" \
      -e "s#@TEST_SESSION_DIR@#$(pwd)#g" \
      -e "s#@TEST_RUNTIME_CONFIG_DIR@#$(pwd)/rtes#" \
      -e "s#@TEST_ARC_LOCATION@#$(dirname $(pwd))#" \
      -e "s#@TEST_HOSTNAME@#$(uname -n)#" \
      "expected_lrms_job_script.tmpl" > ${test}.expected_job_options_in_lrms_script

  # Compare (diff) expected LRMS job options with those from job script.
  TESTFAILED=0
  diffOutput=$(diff -ubBw ${test_ignore_matching_line} ${test}.expected_job_options_in_lrms_script ${lrms_script_name})
  if test $? != 0; then
    TESTFAILED=1
    echo -n "F"
    exitCode=$((exitCode + 1))
    errorOutput="$errorOutput"$'\n\n'"Test fail in test_${test}:"$'\n'"${diffOutput}"
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
