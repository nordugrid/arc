include ../../../Make.inc
include ../../../Make.rules

PROGRAMS=submit-fork-job cancel-fork-job scan-fork-job

all: ${PROGRAMS}

scan-fork-job: scan-fork-job.in
	@cat scan-fork-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > scan-fork-job
	@chmod a+x scan-fork-job

cancel-fork-job: cancel-fork-job.in
	@cat cancel-fork-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > cancel-fork-job
	@chmod a+x cancel-fork-job

submit-fork-job: submit-fork-job.in
	@cat submit-fork-job.in | sed '\
	 s#@arc_location@#${ARC_LOCATION}#g;\
	 s#@tmp_dir@#${TMP_DIR}#g;\
	 s#@gnu_time@#${GNU_TIME}#g;\
	 s#@nodename@#${NODENAME}#g;\
	 s#@runtime_config_dir@#${RUNTIME_CONFIG_DIR}#g;\
	 s#@runtime_local_scratch_dir@#${RUNTIME_LOCAL_SCRATCH_DIR}#g;\
	 s#@runtime_frontend_sees_node@#${RUNTIME_FRONTEND_SEES_NODE}#g;\
	' > submit-fork-job
	@chmod a+x submit-fork-job

clean:
	@rm -f ${PROGRAMS}

install: ${PROGRAMS}
	@echo "Actuall installation is performed in base directory"
