dist_pkgdata_DATA = config_parser.sh config_parser_compat.sh cancel_common.sh
pkgdata_DATA = submit_common.sh scan_common.sh

if PYTHON_LRMS_ENABLED
PYTHON_LRMS = arc
else
PYTHON_LRMS =
endif

SUBDIRS = pbs fork sge condor lsf ll slurm dgbridge boinc test $(PYTHON_LRMS)

DIST_SUBDIRS = pbs fork sge condor lsf ll slurm dgbridge boinc test \
	       arc
