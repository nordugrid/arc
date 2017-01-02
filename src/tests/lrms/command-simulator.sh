#!/bin/bash
#
# Script to simulate a given command. Read 'instructions' from outcome file, and
# acts accordingly. Each time this script is run, the outcome file is checked
# for content. If no content exist, script outputs ${SUCCESS_OUTPUT} and exits
# with code 0. Otherwise the first line from the outcome file is read,
# 'eval'-ed and then removed from the file.
#

if test ! -f ${SIMULATOR_OUTCOME_FILE}; then
  SIMULATOR_OUTCOME_FILE="simulator-outcome.dat"
fi

if test -z ${SIMULATOR_ERRORS_FILE}; then
  SIMULATOR_ERRORS_FILE="simulator-errors.dat"
fi

cmd="$(basename ${0})"
cmd_n_args="${cmd} ${*}"

if test -f "${SIMULATOR_OUTCOME_FILE}"; then
  # Extract rargs from outcome file in order to be able to do reqular expresssion matching.
  # If rargs matches cmd_n_args, use rargs as key at lookup in outcom file.
  while read rargs; do
    echo "${cmd_n_args}" | sed -n -e "${rargs} q 100"
    if test ${?} == 100 && test "x${rargs}" != "x"; then
      # Use rargs as key.
      cmd_n_args="${rargs}"
      break
    fi
  done <<< "$(sed -n -e "/^[[:space:]]*rargs=\"[^\"]*\"/ s/^[[:space:]]*rargs=\"\([^\"]*\)\".*/\1/ p" "${SIMULATOR_OUTCOME_FILE}")"
  # Do not pipe output into while loop, because it creates a subshell, and then setting cmd_n_args has no effect.

  # Escape special characters so they are not interpretted by sed at lookup.
  cmd_n_args="$(echo "${cmd_n_args}" | sed -e 's/[][\/$*.^|]/\\&/g' -e 's/[(){}]/\\\\&/g')"

  # Lookup cmd_n_args in outcome file, and return corresponding options.
  outcome="$(sed -n -e '
    /^[[:space:]]*r\?args="'"${cmd_n_args}"'"/ {
      :ARGS n
      /^[[:space:]]*\(#\|$\)/ b ARGS
      /^[[:space:]]*\(sleep\|rc\)=/ {p; b ARGS}
      /^[[:space:]]*output=<<<\([[:alnum:]]\+\)/ {
        :OUTPUT N
        /output=<<<\([[:alnum:]]\+\)\n.*\n\1$/ ! b OUTPUT
        s/output=<<<\([[:alnum:]]\+\)\(\n.*\n\)\1/read -r -d "" output <<"\1"\2\1/ p
        q 100
      }
      /^[[:space:]]*output="[^\"]*"/ {p; q 100}
      q 50
    }' ${SIMULATOR_OUTCOME_FILE})"
  sed_rc=${?}

  if test ${sed_rc} == 50; then
    printf "Syntax error in simulator outcome file ('%s') options for arguments '%s'\n" "${SIMULATOR_OUTCOME_FILE}" "${cmd_n_args}" >> ${SIMULATOR_ERRORS_FILE}
    exit 1
  fi
  if test "${cmd}" == "sleep" && test ${sed_rc} != 100; then
    # Do not sleep - only if instructed to in SIMULATOR_OUTCOME_FILE
    exit 0
  fi
  if test ${sed_rc} != 100; then
    echo "Command '${cmd} ${@}' was not expected to be executed." >> ${SIMULATOR_ERRORS_FILE}
    exit 1
  fi

  eval "${outcome}"

  if test ! -z "${output+yes}"; then
    echo "${output}"
  else
    echo "${SUCCESS_OUTPUT}"
  fi
  if test ! -z "${sleep+yes}"; then
    /bin/sleep ${sleep}
  fi
  if test ! -z "${rc+yes}"; then
    exit ${rc}
  else
    exit 0
  fi
else
  echo "${SUCCESS_OUTPUT}"
  exit 0
fi
