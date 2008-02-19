include ../../../Make.inc
include ../../../Make.rules

PROGRAMS=cancel-condor-job finish-condor-job

all: ${PROGRAMS}

cancel-condor-job: cancel-condor-job.in
	@cat cancel-condor-job.in | sed '\
	 s#@nordugrid_location@#${ARC_LOCATION}#g;\
	' > cancel-condor-job
	@chmod a+x cancel-condor-job

finish-condor-job: finish-condor-job.in
	@cat finish-condor-job.in | sed '\
	 s#@nordugrid_location@#${ARC_LOCATION}#g;\
	' > finish-condor-job
	@chmod a+x finish-condor-job



clean:
	@rm -f ${PROGRAMS}

install: ${PROGRAMS}
	@echo "Actuall installation is performed in base directory"
