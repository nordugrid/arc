LRMS shell-backends overview for developers 
*******************************************

CONFIG variables used in LRMS shell-backend:
--------------------------------------------

submit_common.sh::

  $CONFIG_runtimedir		[arex]
  $CONFIG_shared_scratch	[arex]
  $CONFIG_shared_filesystem	[arex]
  $CONFIG_scratchdir		[arex]
  $CONFIG_defaultmemory		[queue] [lrms]
  $CONFIG_nodememory		[queue] [infosys/cluster]

lrms=boinc::

  $CONFIG_boinc_app_id   # not in reference
  $CONFIG_boinc_db_host  [lrms]
  $CONFIG_boinc_db_port  [lrms]
  $CONFIG_boinc_db_user  [lrms]
  $CONFIG_boinc_db_pass  [lrms]
  $CONFIG_boinc_db_name  [lrms]

lrms=condor::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)
  
  $CONFIG_condor_requirements 	[queue] [lrms]
  $CONFIG_condor_rank			[lrms]
  $CONFIG_shared_filesystem		[arex]
  $CONFIG_condor_bin_path		[lrms]
  $CONFIG_condor_config			[lrms]

lrms=fork::

  no variables

lrms=ll::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_ll_bin_path			[lrms]
  $CONFIG_ll_consumable_resources	[lrms]
  $CONFIG_ll_parallel_single_jobs	*not in reference
  $CONFIG_scratchdir			[arex]

lrms=lsf::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_lsf_architecture		[lrms]
  $CONFIG_lsf_bin_path			[lrms]

lrms=pbs::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_pbs_queue_node        [queue]
  $CONFIG_pbs_bin_path			[lrms]
  $CONFIG_nodememory			[queue] [infosys/cluster]
  $CONFIG_pbs_log_path			[lrms]
  $CONFIG_shared_filesystem		[arex]

lrms=sge::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)
  $CONFIG_sge_root			    [lrms]
  $CONFIG_sge_cell			    [lrms]
  $CONFIG_sge_qmaster_port		[lrms]
  $CONFIG_sge_execd_port		[lrms]
  $CONFIG_sge_bin_path			[lrms]
  $CONFIG_sge_jobopts			[queue] [lrms]
  $CONFIG_scratchdir			[arex]

lrms=slurm::

  $CONFIG_enable_perflog_reporting	[common] not in reference
  $CONFIG_perflogdir			[common] not in reference
  $CONFIG_controldir			[arex] (for perflog)

  $CONFIG_slurm_wakeupperiod	[lrms]
  $CONFIG_slurm_use_sacct		[lrms]
  $CONFIG_shared_filesystem		[arex]
  $CONFIG_slurm_bin_path		[lrms]


Call graph
----------

Submitting jobs
~~~~~~~~~~~~~~~

.. graphviz::

   digraph {
       subgraph cluster_0 {
          node [style=filled, shape=Rectangle];
          label = "sumbit_LRMS_job.sh";
          "define joboption_lrms" -> "source lrms_common.sh" -> "source submit_common.sh";
          "source submit_common.sh" -> "common_init" -> lslogic;
          lslogic [ label="LRMS-specific submit" ];
       }

       subgraph cluster_1 {
          label = "sumbit_common.sh";
          style = "dashed";
          node [style=filled];
          "common_init()";
          aux1 [ label="RTEs()" ];
          aux2 [ label="Moving files()" ];
          aux3 [ label="I/O redicrection()" ];
          aux4 [ label="Defining user ENV()" ];
          aux5 [ label="Memory requirements()" ];
          aux1 -> lslogic;
          aux2 -> lslogic;
          aux3 -> lslogic;
          aux4 -> lslogic;
          aux5 -> lslogic;
          # rank hack
          aux1 -> aux2 -> aux3 -> aux4 -> aux5 [style=invis];
        }

        subgraph cluster_2 {
           label = "lrms_common.sh";
           style = "dashed";
           node [style=filled];
          "packaging paths" -> "source lrms_common.sh";
          "parse_arc_conf()" -> "common_init()";
          "parse_grami()" -> "common_init()";
          "init_lrms_env()" -> "common_init()";
          "packaging paths" [shape=Rectangle]
        }

        subgraph cluster_3 {
          label = "configure_LRMS_env.sh";
          node [style=filled, shape=Rectangle];
          "set LRMS-specific ENV/fucntions"  -> "common_init()";
        }

        "a-rex" -> "define joboption_lrms";
        "common_init()" -> "common_init"

        "arc.conf" -> "parse_arc_conf()";
        "grami file" -> "parse_grami()";

        # rank hack
        "packaging paths" -> "set LRMS-specific ENV/fucntions" [style=invis];

        "a-rex" [shape=Mdiamond];
        "grami file" [shape=Msquare];
        "arc.conf" [shape=Msquare];
        lslogic -> "LRMS";
        "LRMS" [shape=Mdiamond];
    }

Scanning jobs
~~~~~~~~~~~~~

.. graphviz::

   digraph {
       subgraph cluster_0 {
          node [style=filled, shape=Rectangle];
          label = "scan_LRMS_job.sh";
          lslogic [ label="LRMS-specific scan" ];
          "define joboption_lrms" -> "source lrms_common.sh" -> "source scan_common.sh";
          "source scan_common.sh" -> "common_init" -> lslogic;
       }

       subgraph cluster_1 {
          label = "scan_common.sh";
          style = "dashed";
          node [style=filled];
          "common_init()";
          aux1 [ label="Timestamp convertion()" ];
          aux2 [ label="Owner UID()" ];
          aux3 [ label="Read/Write diag()" ];
          aux4 [ label="Save commentfile()" ];
          aux1 -> lslogic;
          aux2 -> lslogic;
          aux3 -> lslogic;
          aux4 -> lslogic;
          # rank hack
          "common_init()" -> aux1 -> aux2 -> aux3 -> aux4 [style=invis];
        }

        subgraph cluster_2 {
           label = "lrms_common.sh";
           style = "dashed";
           node [style=filled];
          "packaging paths" -> "source lrms_common.sh";
          "parse_arc_conf()" -> "common_init()";
          "init_lrms_env()" -> "common_init()";
          "parse_grami()";
          "packaging paths" [shape=Rectangle]
        }

        subgraph cluster_3 {
          label = "configure_LRMS_env.sh";
          node [style=filled, shape=Rectangle];
          "set LRMS-specific ENV/fucntions"  -> "common_init()";
        }

        "a-rex" -> "define joboption_lrms";
        "common_init()" -> "common_init"

        "arc.conf" -> "parse_arc_conf()";
        "controldir" -> lslogic;
        lslogic -> "LRMS";

        # rank hack
        "source lrms_common.sh" -> "set LRMS-specific ENV/fucntions" [style=invis];

        "a-rex" [shape=Mdiamond];
        "controldir" [shape=Msquare];
        "arc.conf" [shape=Msquare];
        "LRMS" [shape=Mdiamond];
    }

Canceling jobs
~~~~~~~~~~~~~~

.. graphviz::

   digraph {
       subgraph cluster_0 {
          node [style=filled, shape=Rectangle];
          label = "cancel_LRMS_job.sh";
          lslogic [ label="LRMS-specific cancel" ];
          "define joboption_lrms" -> "source lrms_common.sh" -> "source scan_common.sh";
          "source scan_common.sh" -> "common_init" -> lslogic;
       }

       subgraph cluster_1 {
          label = "cancel_common.sh";
          style = "dashed";
          node [style=filled];
          "common_init()";
        }

        subgraph cluster_2 {
           label = "lrms_common.sh";
           style = "dashed";
           node [style=filled];
          "packaging paths" -> "source lrms_common.sh";
          "parse_arc_conf()" -> "common_init()";
          "init_lrms_env()" 
          "parse_grami()" -> "common_init()";
          "packaging paths" [shape=Rectangle]
        }

        subgraph cluster_3 {
          label = "configure_LRMS_env.sh";
          node [style=filled, shape=Rectangle];
          "set LRMS-specific ENV/fucntions"  -> "common_init()";
        }

        "a-rex" -> "define joboption_lrms";
        "common_init()" -> "common_init"

        "arc.conf" -> "parse_arc_conf()";
        "grami file" -> "parse_grami()";
        lslogic -> "LRMS";

        # rank hack
        "source lrms_common.sh" -> "set LRMS-specific ENV/fucntions" [style=invis];

        "a-rex" [shape=Mdiamond];
        "grami file" [shape=Msquare];
        "arc.conf" [shape=Msquare];
        "LRMS" [shape=Mdiamond];
    }


Changes in ARC6 memory limits processing:
-----------------------------------------

Current logic of memory limits processing:

  * ``nodememory`` - advertise memory for matchmaking: max memory on the nodes (in ``[infosys/cluster]`` block or per-queue)
  * ``defaultmemory`` - enforce during submission if no memory limit specified in the job description (in ``[lrms]`` block or per-queue)

The ARC6 logic is *no enforcement = no limit* [1]_

.. [1] ARC5 logic was *no enforcement = max node memory* or 1GB if ``nodememory`` is not published (and not used for matchmaking)

Backends behaviour with no memory enforcement limit:
  * boinc - set to hardcoded 2GB
  * condor - no enforcement
  * form - no memory handling at all
  * ll - no enforcement
  * lsf - no enforcement
  * pbs - no enforcement [2]_ 
  * sge - no enforcement
  * slurm - no enforcement

.. [2] exclusivenode is memory-based and ``nodememory`` value is used in this case


