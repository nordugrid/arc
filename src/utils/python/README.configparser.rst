General arc.conf python configuration parser
############################################

Parsing configuration
*********************

.. _initial_parsing:

Initial configuration parsing
+++++++++++++++++++++++++++++

The latest version of ``arcconfig-parser`` designed to operate with defaults file that holds default values for all
possible parameters.

At the time of initial parsing the following chain is executed:

 * All blocks and options are parsed from ``arc.conf``
 * For all blocks defined in ``arc.conf`` missing options are added from defaults file
 * Special constructs in values are substituted (see :ref:`special_constructs`)

Optional by design parameters that does not have default value (specified with ``not set`` value) are not included
to the parsed configuration.

Runtime configuration
+++++++++++++++++++++

Configuration that includes both ``arc.conf`` and defaults config called *runtime configuration*.

In some cases it is useful to save and load runtime configuration:

 * To supply C++ services (``a-rex``, ``gridftpd``) with configuration that includes defaults from common place
 * For repetitive operations on config to eliminate full-chain processing of each invocation

To save runtime configuration to the default location (``/var/run/arc/``)::

  arcconfig-parser --save

To save runtime configuration to specified location::

  arcconfig-parser --save -r /var/run/arc/arex.arc.conf

To load runtime configuration instead of full-chain processing and e.g. get the value of ``x509_host_key`` in ``[common]`` block::

  arcconfig-parser --load -b common -o x509_host_key


.. _special_constructs:

Special constructs can be used as values
++++++++++++++++++++++++++++++++++++++++
Defaults includes references to another config parts to be consistent with the implied ``arc.conf`` structure.

The most obvious example is if ``x509_host_key`` not found in e.g. ``[arex/jura]`` block
it should be taken from ``[common]``.

Config parser is following this logic (especially in respect to defaults) and use special
constructs to accomplish this behaviour.

Command substitutions
---------------------
Configuration option values can contain the construct ``$EXEC{<command>}`` that substituted to the stdout of ``<command>``.

For example::

  hostname=$EXEC{hostname -f}

Option values substitutions
---------------------------
The construct ``$VAR{[block]option}`` can be used to substitute the values of another option value.

If option is in the same block as referencing option block name can be omitted - ``$VAR{option}``.

For example::

  x509_host_key=$VAR{[common]x509_host_key}
  bdii_update_cmd=$VAR{bdii_location}/sbin/bdii-update

Evaluation of simple code
-------------------------
For limited number of cases ``arc.conf`` default values relies on arithmetic operations. For this purpose the
``$EVAL{string}`` special construct had been introduced.

For example::

  bdii_read_timeout=$EVAL{$VAR{provider_timeout} + $VAR{[arex]infoproviders_timelimit} + $VAR{[arex]wakeupperiod}}

Getting the configuration values
++++++++++++++++++++++++++++++++

If ``--option`` argument is passed to ``arcconfig-parser`` parser returns the value of the specified option to stdout.

Without ``--option`` ``arcconfig-parser`` can be used to operate with configuration blocks:
 * check blocks existance (exit code used to indicate the status of the check)
 * return the list of subblocks

With the ``--export`` option ``arcconfig-parser`` allows to export config in the following formats:
 * ``json`` - returns entire configuration or subset of blocks as-is in JSON to stdout
 * ``bash`` - for ``[common]`` block or specified configuration subset returns ``CONFIG_option_name=value`` pairs to stdout.
   Block names ARE NOT included in the exports and option values precedence will be used in the order of passed blocks.
   If automatic subblocks expansion used with bash export, for every block in sequence - it's subblocks are processed
   first (in ``arc.conf`` defined order).

Common configuration parsing sequence
+++++++++++++++++++++++++++++++++++++

.. graphviz::

  digraph {
     node [shape=Rectangle];
     forcelabels=true;
     reference [label="arc.conf.reference", xlabel="Developers entry-point to put info"];
     subst [label="Substitution syntax", style=filled];
     reference -> subst [dir=back];
     {rank = same; subst; reference;}
     reference -> "arc.parser.defaults" [ label="buildtime", fontcolor=red ];

     subgraph cluster_0 {
        style = "dashed";
        color = "red";
        label = "Binary Distribution";
        doc [ label="/usr/share/doc" ];
        arcconf [ label="/etc/arc.conf"];
        defconf [ label="/usr/share/arc/parser.defaults" ];
        # hack rank
        arcconf -> defconf [style=invis];
     }

     reference -> doc;
     "arc.parser.defaults" -> defconf;
    
     subst -> "arc.parser.defaults"
     subst -> defconf;

     parser [ label="arcconfig-parser" ];
     arcconf -> parser [label="1. parse, get defined blocks"]
     defconf -> parser [label="2. add defaults for defined blocks"]

     runconfig [ label="runtime configuration", shape=oval, color=red ]
     parser -> runconfig [ label="3. evaluate substitutions" ]

     json [label="export JSON"]
     bash [label="export BASH"]
     check [label="get value"]

     runconfig -> json
     runconfig -> bash
     runconfig -> check

     subgraph cluster_1 {
       label = "Startup scripts"
       runconf [ label="/var/run/arc/arc.conf" ];
       define [ label="define ENV variables" ]
       arex [ label="start a-rex" ]
       runconf -> define -> arex
     }

     runconfig -> runconf [ label="dump config" ]
  }


Examples
********

Get value of option in block::

  # arcconfig-parser --block infosys --option providerlog
  /var/log/arc/infoprovider.log

Get value of option in blocks in order they are specified
(e.g. if not found in ``[gridftpd]`` look in the ``[common]`` block [1]_)::

  # arcconfig-parser --block gridftpd --block common --option x509_user_key
  /etc/grid-security/hostkey.pem

.. [1] Block dependencies are now implied by defaults file, so for most cases it is enough to specify only block in question

Export entire configuration to JSON [2]_::

  # arcconfig-parser --export json

.. [2] *HINT:* use ``arcconfig-parser --export json | jq .`` to view highlighted JSON structure in shell

Export ``[infosys]`` block options to JSON (for Perl)::

  # arcconfig-parser --block infosys --export json
  {"infosys": {"registrationlog": "/var.....

Export ``[infosys]`` block and all their subblocks options to JSON::

  # arcconfig-parser --block infosys --subblocks --export json
  {"infosys/admindomain": {"www": "http://e....

Export for BASH (compatible with current config representation in shell-based LRMS backends)::

  # arcconfig-parser --block infosys --block arex --block common --export bash
  CONFIG_controldir="/var/spool/arc/jobstatus"
  CONFIG_defaultttl="1210000"
  CONFIG_delegationdb="sqlite"
  CONFIG_hostname="sample1.nordugrid.org"
  CONFIG_maaxrerun="5"
  CONFIG_maxjobs="10000 -1"
  CONFIG_runtimedir="/home/grid/arc/runtime"
  CONFIG_sessiondir="__array__" # <= NEW define for multivalued values that indicate indexed vars
  CONFIG_sessiondir_0="/mnt/scratch/grid/arc/session"
  CONFIG_sessiondir_1="/home/grid/arc/session drain"
  ...

Using BASH export::

  # eval "$( arcconfig-parser --block infosys --block arex --block common --export bash )"
  # echo "$CONFIG_gridmap"

Check block(s) exists (``[common/perflog]`` is not exists in the example)::

  # arcconfig-parser --block common/perflog --block arex
  # echo $?
  1

List block subblocks::

  # arcconfig-parser --block infosys --subblocks
  infosys
  infosys/ldap
  infosys/ldap/bdii
  infosys/nordugrid
  infosys/glue2
  infosys/glue2/ldap
  infosys/glue1

Using parser as Python module::

  from arc.utils import config

  # initial parsing with defaults
  config.parse_arc_conf('/tmp/arc.conf', '/tmp/defaults.conf')

  # get parsed dictionary and list of blocks in the arc.conf order
  >>> confdict = config.get_config_dict()
  >>> confblocks = config.get_config_blocks()

  # get list of all [queue] subblocks sorted by name
  >>> sb = config.get_subblocks(['queue'], is_sorted=True)
  >>> sb
  ['queue:grid', 'queue:grid_rt']

  # get value of 'x509_host_key' from [arex] block and than from [common] if not found in [arex]
  >>> a = config.get_value('x509_host_key', ['arex', 'common'])
  >>> a
  '/etc/grid-security/hostkey.pem'

  # get value of 'allowunknown' option from [gridftpd] block
  >>> b = config.get_value('allowunknown', 'gridftpd')
  >>> b
  'yes'

  # get value of 'allowunknown' option from [gridftpd] block (always return list)
  >>> c = config.get_value('allowunknown', 'gridftpd', force_list=True)
  >>> c
  ['yes']

  # get value of 'allowunknown' option from [gridftpd] block (return boolean value)
  >>> d = config.get_value('allowunknown', 'gridftpd', bool_yesno=True)
  >>> d
  True

