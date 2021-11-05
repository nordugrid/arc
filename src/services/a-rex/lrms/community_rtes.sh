COMMUNITY_RTES=1
COMMUNITY_RTES_SW_SUBDIR="_software"

community_software_prepare () {
  # skip if this is not a community-defined RTE
  [ -e "${rte_params_path}.community" ] || return
  # source community deploy-time parameters
  ". ${rte_params_path}.community" 1>&2
  # check software directotry is defined
  if [ -z "${SOFTWARE_DIR}" ]; then
    echo "ERROR: SOFTWARE_DIR is not defined for ${rte_name} community RTE. Failed to prepare software files." 1>&2
    exit 1
  fi
  # software location in the sessiondir
  RUNTIME_SOFTWARE_DIR="${joboption_directory}/${COMMUNITY_RTES_SW_SUBDIR}/${rte_name}"
  mkdir -p "${RUNTIME_SOFTWARE_DIR%/*}"
  # copy software if not shared, link if shared
  if [ "$SOFTWARE_SHARED" != "True" ]; then
    echo "Copying community software for RTE ${rte_name} into job directory." 1>&2
    cp -rv "${SOFTWARE_DIR}" "${RUNTIME_SOFTWARE_DIR}"
  else
    echo "Linking community software for RTE ${rte_name} into job directory." 1>&2
    ln -sf "${SOFTWARE_DIR}" "${RUNTIME_SOFTWARE_DIR}"
  fi
  # define ${RUNTIME_JOB_SWDIR} for RTE stage 0
  RUNTIME_JOB_SWDIR="\${RUNTIME_JOB_DIR}/${COMMUNITY_RTES_SW_SUBDIR}/${rte_name}"
}

community_software_environment () {
  # skip if this is not a community-defined RTE
  [ -e "${rte_params_path}.community" ] || return
  # define RUNTIME_JOB_SWDIR for RTE stage 1/2 in the jobscript
  echo "RUNTIME_JOB_SWDIR=\"\${RUNTIME_JOB_DIR}/${COMMUNITY_RTES_SW_SUBDIR}/${rte_name}\""
}
