check_any_scratch () { :; }
RTE_stage0 () { :; }
RTE_stage1 () { echo '# TEST_RTE_STAGE1' >> "$LRMS_JOB_SCRIPT"; }
RTE_stage2 () { :; }
mktempscript () {
    LRMS_JOB_SCRIPT=$TEST_ROOT/job.script
    LRMS_JOB_OUT=$TEST_ROOT/qsub.stdout
    LRMS_JOB_ERR=$TEST_ROOT/qsub.stderr
}
set_count () { :; }
set_req_mem () { :; }
sourcewithargs_jobscript () { :; }
accounting_init () { :; }
accounting_end () { :; }
add_user_env () { :; }
setup_runtime_env () { :; }
include_std_streams () { :; }
move_files_to_node () { :; }
move_files_to_frontend () { :; }
clean_local_scratch_dir_output () { :; }
detect_wn_systemsoftware () { :; }
cd_and_run () { :; }
