Configuration variables used in LRMS shell-backends
***************************************************

General CONFIG variables search:
--------------------------------

submit_common.sh::

  $CONFIG_runtimedir		[arex]
  $CONFIG_shared_scratch	[arex]
  $CONFIG_shared_filesystem	[arex]
  $CONFIG_scratchdir		[arex]
  $CONFIG_defaultmemory		[lrms]
  $CONFIG_nodememory		[queue] [cluster]

lrms=boinc::

  $CONFIG_boinc_app_id    # not in reference
  $CONFIG_boinc_db_host  [lrms]
  $CONFIG_boinc_db_port  [lrms]
  $CONFIG_boinc_db_user  [lrms]
  $CONFIG_boinc_db_pass  [lrms]
  $CONFIG_boinc_db_name  [lrms]

lrms=condor::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_controldir			[arex] (for perflog)
  
  $CONFIG_condor_requirements 		[lrms]
  $CONFIG_condor_rank			[lrms]
  $CONFIG_shared_filesystem		[arex]
  $CONFIG_condor_bin_path		[lrms]
  $CONFIG_condor_config			[lrms]

lrms=dgbridge::

  $CONFIG_dgbridge_stage_dir		[lrms]
  $CONFIG_dgbridge_stage_prepend	[lrms]
  $CONFIG_scan_wakeupperiod		* not in reference [default: 30]

lrms=fork::

  no variables

lrms=ll::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_ll_bin_path			[lrms]
  $CONFIG_ll_consumable_resources	[lrms]
  $CONFIG_ll_parallel_single_jobs	*not in reference
  $CONFIG_scratchdir			[arex]

lrms=lsf::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_lsf_architecture		[lrms]
  $CONFIG_lsf_bin_path			[lrms]

  [lrms]lsf_profile_path is in .reference but not used in scripts

lrms=pbs::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_queue_node_string		*not in reference
					but pbs_dedicated_node_string is
  $CONFIG_pbs_bin_path			[lrms]
  $CONFIG_nodememory			[queue] [cluster]
  $CONFIG_controldir			[arex] (for perflog)
  $CONFIG_pbs_log_path			[lrms]
  $CONFIG_shared_filesystem		[arex]
  
  maui_bin_path is not used		[lrms]

lrms=sge::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_controldir			[arex] (for perflog)
  $CONFIG_sge_root			[lrms]
  $CONFIG_sge_cell			[lrms]
  $CONFIG_sge_qmaster_port		[lrms]
  $CONFIG_sge_execd_port		[lrms]
  $CONFIG_sge_bin_path			[lrms]
  $CONFIG_sge_jobopts			[lrms]
  $CONFIG_scratchdir			[arex]

lrms=slurm::

  $CONFIG_enable_perflog_reporting	[common]? not in reference
  $CONFIG_perflogdir			[common]? not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_slurm_wakeupperiod		[lrms]
  $CONFIG_slurm_use_sacct		[lrms]
  $CONFIG_shared_filesystem		[arex]
  $CONFIG_slurm_bin_path		[lrms]

Memory limits processing:
-------------------------

The nodememory processing is the reason of parsing the [cluster] block in LRMS backends. Lets go deep into this problem.

The is no ``defaultmemory`` in [queue] configuration, only ``nodememory``.

Some logic that should be in configuration:

  * nodememory -> advertise for matchmaking (max memory on the nodes)
  * defaultmemory -> enforce during sumission is no memory limit in job descritpion

The logic should be *no enforcement = no limit*

But now for ARC logic is *no enforcement = max node memory* or 1GB if nodememory not publisehd and not used by matchmaking.

Without meaningfull ``defaultmemory`` or memory limit in job description this leads to whole node consumption by a single job.

This strange enforcement is in the ``submit_common.sh`` and ``$joboption_memory`` always set to some value.

Analyzing what backends do with ``$joboption_memory``:

  * boinc - if empty - set to hardcoded 2GB
  * condor - if empty - no enforcement
  * dgbridge - no memory handling at all
  * form - no memory handling at all
  * ll - if empty - no enforcement
  * lsf - if empty - no enforcement
  * pbs - if empty - no enforcement [1]_ 
  * sge - if empty - no enforcement
  * slurm - if empty - no enforcement

.. _[1] but exclusivenode is memory-based and code requires some adjustments to eliminate errors in log in case the joboption_memory is not set

