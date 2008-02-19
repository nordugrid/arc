include ../../../Make.inc
include ../../../Make.rules

PROGRAMS=submit-pbs-job cancel-pbs-job scan-pbs-job

all: ${PROGRAMS}

cancel-pbs-job: cancel-pbs-job.in
	@cat cancel-pbs-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@pbs_bin_path@#${PBS_BIN_PATH}#g;\
	 s#@pbs_log_path@#${PBS_LOG_PATH}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	 s#@runtime_node_sees_frontend@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > cancel-pbs-job
	@chmod a+x cancel-pbs-job

submit-pbs-job: submit-pbs-job.in
	@cat submit-pbs-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@pbs_bin_path@#${PBS_BIN_PATH}#g;\
	 s#@pbs_log_path@#${PBS_LOG_PATH}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	 s#@runtime_node_sees_frontend@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > submit-pbs-job
	@chmod a+x submit-pbs-job

scan-pbs-job: scan-pbs-job.in
	@cat scan-pbs-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@pbs_bin_path@#${PBS_BIN_PATH}#g;\
	 s#@pbs_log_path@#${PBS_LOG_PATH}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	 s#@runtime_node_sees_frontend@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > scan-pbs-job
	@chmod a+x scan-pbs-job



clean:
	@rm -f ${PROGRAMS}

install: ${PROGRAMS}
	@echo "Actuall installation is performed in base directory"
