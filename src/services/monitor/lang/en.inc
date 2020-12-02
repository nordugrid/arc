<?php

// ------ Labels: strings, titles, error messages etc, locale-dependent
// -- DEFAULT (mandatory for the Monitor to work)
// -- English [en]
// -- Author: oxana.smirnova@hep.lu.se

// Some common strings:

$clickable = "CLICKABLE";

$str_att = ", human-readable except of some MDS-specific attributes. Click on the attribute name
to get the list of the attribute values across the ARC universe.";

$str_val = "Attribute value as entered in the Information System.";

$str_que = "Typically, different queues correspond to different allowed task duration, or to different groups of users. Click on a queue name to get detailed information about the queue, including running, queued, and finished tasks.";

$str_job = "Name of a job as assigned by the job owner.
          <br> <font color=\"red\">N/A</font> indicates that user did not assign any name.
          <br> <font color=\"red\"><b>X</b></font> indicates that the job has been killed by the owner
          <br> <font color=\"red\"><b>!</b></font> indicates that the job failed in the system
          <br> Click on a name to get a detailed description of the job.";

$str_nam = "Name of the user, as specified in the personal certificate. Click on a name to get the list of all the resources available for this user and all the jobs by this user which are currently in the system.";

$str_sta = "Job status as returned by the Grid Manager (GM) and
   LRMS. In sequential order, the states are:
   <dl>
     <dt>ACCEPTED &ndash; job submitted but not yet processed</dt>
     <dt>PREPARING &ndash; input files are being retreived</dt>
     <dt>SUBMITTING &ndash; interaction with LRMS ongoing</dt>
     <dt>INLRMS &ndash; the job is transferred to the LRMS; internal
       status is added by the infosystem. Possible states are:</dt>
       <dd>: Q &ndash; job is queued
       <br>: U &ndash; job is in a suspended state on a busy node (PBSPro)
       <br>: S &ndash; job is in a suspended state (Condor)
       <br>: R, run &ndash; job is running
       <br>: E &ndash; job is finishing (PBS)
     </dd>
     <dt>FINISHING &ndash; output files are being transferred by the GM</dt>
     <dt>FINISHED &ndash; job is finished; time stamp is added by the infosystem</dt>
     <dt>CANCELING &ndash; job is being cancelled</dt>
     <dt>DELETED &ndash; job not cleaned upon user request but
       removed by the GM due to expiration time</dt>
   </dl>
   Each of the states can be reported with the PENDING: prefix,
   meaning the GM is attempting to move the job to the next
   state";

$str_tim = "CPU time used by the job, minutes.";

$str_mem = "Memory consumed by the job, KB";

$str_cpu = "Number of processors used by the job.";

// Actual messages

$message = array (
                  // Table headers and help (do not localize "0" and "help" keys)
                  // For "help", keywords in <dt></dt> must correspond to column titles below the help text )
                  "loadmon"  => array(
                                      "0" => "Grid Monitor",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt>
                                                    This screen displays all the sites registering to the top ARC
                                                    indexing service, sorted by country then by host name. Selected site
                                                    parameters are monitored: cluster alias, total CPU capacity and number
                                                    of running and queued jobs, both Grid and local ones. Use &quot;Search&quot;
                                                    utility if you want to compare other cluster, queue, job etc. characteristics
                                                  </dt>
                                                  <dt><b>Country</b></dt>
                                                  <dd>".$clickable.". Country flag and name as deduced from available resource
                                                      descriptions. Click to show only this country info. </dd>
                                                  <dt><b>Cluster</b></dt>
                                                  <dd>".$clickable.". Cluster alias as assigned by the owner. Maximal displayed
                                                      length is 22 characters. Click on the alias to get a detailed cluster
                                                      description.</dd>
                                                  <dt><b>CPUs</b></dt>
                                                  <dd>Total number of CPUs in a cluster. <b>NB!</b> Only a fraction of those
                                                      can be actualy available for the Grid users.</dd>
                                                  <dt><b>Load (processes:Grid&#43local)</b></dt>
                                                  <dd>".$clickable.". Relative cluster load, corresponding to the occupied CPUs
                                                      count. Grey bars indicate processors occupied by the localy submitted
                                                      jobs, while red bars show CPUs occupied by jobs submitted via Grid. Click
                                                      on the bar to get the detailed list of all the running Grid jobs on the
                                                      cluster, including amount of processors per job.</dd>
                                                  <dt><b>Queueing</b></dt>
                                                  <dd>".$clickable.". Number of all queued jobs on the cluster, shown as number of
                                                      queueing grid jobs plus number of locally submitted queueing jobs. Click
                                                      the first number to get the list of queued Grid jobs on the cluster.</dd>
                                                </dl>
                                                </div>",
                                      "Country"                      => 30,
                                      "Site"                         => 160,
                                      "CPUs"                         => 10,
                                      "Load (processes: Grid+local)" => 210,
                                      "Queueing"                     => 10
                                      ),
                  "clusdes"  => array("0" => "Resource Details for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Attribute</b></dt>
                                                  <dd>".$clickable.". Cluster attribute name".$str_att."</dd>
                                                  <dt><b>Value</b></dt>
                                                  <dd>".$str_val."</dd>
                                                  <dt><b>Queue</b></dt>
                                                  <dd>".$clickable.". Names of batch queues available for the
                                                    ARC users, as set by cluster owners. ".$str_que."</dd>
                                                  <dt><b>Status</b></dt>
                                                  <dd>Queue status. Operating queue typically reports
                                                    <i>active</i> status.</dd>
                                                  <dt><b>CPU (min)</b></dt>
                                                  <dd>Time limit for job duration per queue, if set, in
                                                    CPU-minutes. First displayed value is the lower limit,
                                                    second - the upper one. If limits are not set (jobs of any
                                                    duration are accepted), <b>N/A</b> tag is shown.</dd>
                                                  <dt><b>Running</b></dt>
                                                  <dd>Number of jobs running in the queue. Total number of jobs
                                                    is shown, with number of <b>processors</b> occupied by
                                                    Grid-submitted jobs displayed in parentheses, e.g. (Grid:
                                                    12). NB! For parallel multiprocessor jobs, number in
                                                    parentheses can be larger than number of jobs.</dd>
                                                  <dt><b>Queing</b></dt>
                                                  <dd>Number of jobs awaiting execution in the queue. Total
                                                    number of jobs is shown, with Grid-submitted jobs displayed
                                                    in parentheses, e.g. (Grid: 235)</dd>
                                                </dl>
                                                </div>
",
                                      "Queue"                        => 0,
                                      "Mapping Queue"                => 0,
                                      "Status"                       => 0,
                                      "Limits (min)"                 => 0,
                                      "CPUs"                         => 0,
                                      "Running"                      => 0,
                                      "Queueing"                     => 0
                                      ),
                  "jobstat"  => array("0" => "Jobs at:Job ID",
                                      "help" => "<div align=\"left\">
                                                <b>JOB LIST:</b>
                                                <dl>
                                                <dt><b>Job name</b></dt>
                                                <dd>".$clickable.". Name of a job as assigned by the owner. If no
                                                  name has been assigned, &quot;<span
                                                    style={color:red;}>N/A</span>&quot; is displayed. Click
                                                  on a name to get a detailed description of the job.</dd>
                                                <dt><b>Owner</b></dt>
                                                <dd>".$clickable.". ".$str_nam."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Name of the batch queue in which the job is
                                                  being executed. ".$str_que."</dd>
                                                <dt><b>CPUs</b></dt>
                                                <dd>".$str_cpu."</dd>
                                                </dl>
                                              </div>
                                              <div align=\"left\"><b>JOB DETAILS:</b>
                                                <dl>
                                                <dt><b>Attribute</b></dt>
                                                <dd>".$clickable.". Job attribute name".$str_att."</dd>
                                                <dt><b>Value</b></dt>
                                                <dd>".$str_val."</dd>
                                               </dl>
                                               </div>
",
                                      "Job name"                     => 0,
                                      "Owner"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Queue"                        => 0,
                                      "CPUs"                         => 0
                                      ),
                  "volist"   => array("0" => "Virtual Organisations",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Virtual Organisation</b></dt>
                                                   <dd>".$clickable.". Group of users, typically sharing common
                                                     activities and resources, authorised at at least one
                                                     ARC-enabled site. Click on the name to get the list of
                                                     group members.</dd>
                                                   <dt><b>Members</b></dt>
                                                   <dd>Number of group members.</dd>
                                                   <dt><b>Served by</b></dt>
                                                   <dd>LDAP server that supports the group membership
                                                     database.</dd>
                                                 </dl>
                                                 </div>
",
                                      "Virtual Organisation"         => 0,
                                      "Members"                      => 0,
                                      "Served by"                    => 0
                                      ),
                  "vousers"  => array("0" => "Grid User Base",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Name</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Affiliation</b></dt>
                                                   <dd>Users home institute as entered by a VO manager. Can be
                                                     empty.</dd>
                                                   <dt><b>E-mail</b></dt>
                                                   <dd>".$clickable.". Users e-mail as entered by a VO manager. Can be
                                                     empty. Click the address to send an e-mail to the user.</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Name"                         => 0,
                                      "Affiliation"                  => 0,
                                      "E-mail"                       => 0
                                      ),
                  "userlist" => array("0" => "Information for",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                <dt><b>Cluster:queue</b></dt>
                                                <dd>".$clickable.". Names of clusters and respective queues
                                                  (separated by a column, &quot;:&quot;) where a user is
                                                  authorized to submit jobs. If a user is not authorized,
                                                  message &quot;<b><font color=\"red\">Not authorised at host
                                                      ...</font></b>&quot; is displayed. Click a cluster name
                                                  to get a detailed cluster description. Click on a queue name
                                                  to get a detailed queue description. </dd>
                                                <dt><b>Free CPUs</b></dt>
                                                <dd>Number of free CPUs available in a given queue for the
                                                  user at this moment of time, optionally appended with the
                                                  upper time limit value (in minutes). For example,
                                                  &quot;3&quot; means 3 CPUs available for a job of unlimited
                                                  running time; &quot;4:360&quot; indicates there are 4 CPUs
                                                  available for jobs not longer than 6 hours; &quot;10:180
                                                  30&quot; means there are 10 CPUs available for jobs not
                                                  exceeding 3 hours, plus 30 CPUs available for jobs of any
                                                  length; &quot;0&quot; means there are no CPUs available at
                                                  the moment, and the jobs will be placed in a waiting
                                                  queue.</dd>
                                                <dt><b>Queued jobs</b></dt>
                                                <dd>Number of user's jobs expected to sit ahead of a new submitted
                                                  job (for this user) in a waiting queue. Number of
                                                  &quot;0&quot; means the job is expected to be executed
                                                  immediately. <b>NB!</b> This is only an estimation, which
                                                  can be overwritten by local policies.</dd>
                                                <dt><b>Free disk (MB)</b></dt>
                                                <dd>Disk space available for the user in a given queue (in
                                                  Megabytes). <b>NB!</b> This is only an estimation, as most
                                                  clusters do not provide fixed disk quotas.</dd>
                                                <dt><b>Job name</b></dt>
                                                <dd>".$clickable.". ".$str_job."</dd>
                                                <dt><b>Status</b></dt>
                                                <dd>".$str_sta."</dd>
                                                <dt><b>CPU (min)</b></dt>
                                                <dd>".$str_tim."</dd>
                                                <dt><b>Cluster</b></dt>
                                                <dd>".$clickable.". Name of the cluster at which the job is being
                                                  executed. Click on a cluster name to get detailed
                                                  information about the cluster.</dd>
                                                <dt><b>Queue</b></dt>
                                                <dd>".$clickable.". Name of the batch queue in which the job is/was
                                                  executed. ".$str_que."</dd>
                                                <dt><b>CPUs</b></dt>
                                                <dd>".$str_cpu."</dd>
                                              </dl>
                                              </div>
",
                                      ""                             => 0,
                                      "Job name"                     => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Cluster"                      => 0,
                                      "Queue"                        => 0,
                                      "CPUs"                         => 0
                                      ),
                  "attlist"  => array("0" => "Attribute values",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Object</b></dt>
                                                   <dd>".$clickable.". Name of the object which attributes are
                                                     displayed. It can be a cluster name, a clusters queue name,
                                                     a job name, a user name etc. Click on the string to get a
                                                     detailed decscription of the object.</dd>
                                                   <dt><b>Attribute</b></dt>
                                                   <dd>For each object, one or more attribute values can be
                                                     listed. Column title is the human-readable attribute name
                                                     (except of some MDS-specific attributes), and the column
                                                     contents are attribute values per object as entered in the
                                                     Information System.</dd>
                                                 </dl>
                                                 </div>",
                                      "Object"                       => 0,
                                      "Attribute"                    => 0
                                      ),
                  "quelist"  => array("0" => "Queue",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Attribute</b></dt>
                                                   <dd>".$clickable.". Name of a queue attribute".$str_att."</dd>
                                                   <dt><b>Value</b></dt>
                                                   <dd>".$str_val."</dd>
                                                   <dt><b>Job name</b></dt>
                                                   <dd>".$clickable.". ".$str_job."</dd>
                                                   <dt><b>Owner</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Status</b></dt>
                                                   <dd>".$str_sta."</dd>
                                                   <dt><b>CPU (min)</b></dt>
                                                   <dd>".$str_tim."</dd>
                                                   <dt><b>Memory (KB)</b></dt>
                                                   <dd>".$str_mem."</dd>
                                                   <dt><b>CPUs</b></dt>
                                                   <dd>".$str_cpu."</dd>
                                                 </dl>
                                                 </div>
",
                                      ""                             => 0,
                                      "Job name"                     => 0,
                                      "Owner"                        => 0,
                                      "Status"                       => 0,
                                      "CPU (min)"                    => 0,
                                      "Memory (KB)"                  => 0,
                                      "CPUs"                         => 0
                                      ),
                  "sestat"   => array("0" => "Storage Elements",
                                      "help" => "<div align=\"left\">
                                                <dl>
                                                  <dt><b>Alias</b></dt>
                                                  <dd>Storage element alias as specified in the Information
                                                    System. Maximal displayed length is 15 characters.</dd>
                                                  <dt><b>Tot. space</b></dt>
                                                  <dd>Total disk space, GB.</dd>
                                                  <dt><dt><b>Free space</b></dt>
                                                  <dd>Disk space available at the moment, GB.</dd>
                                                  <dt><b>Name</b></dt>
                                                  <dd>Storage element name, consisting of a logical name and
                                                    host name (separated by a column, &quot;:&quot;). Logical
                                                    name is used only for information system purposes, to
                                                    distinguish between different storage elements hosted by the
                                                    same machine.</dd>
                                                  <dt><b>Base URL</b></dt>
                                                  <dd>URL for the storage element, typically a gsiftp://
                                                    protocol. Use this URL as the base to access files.</dd>
                                                  <dt><b>Type</b></dt>
                                                  <dd>Storage element type. &quot;gridftp-based&quot; indicates
                                                    a disk storage with GridFTP interface.</dd>
                                                </dl>
                                                </div>
",
                                      "#"                            => 0,
                                      "Alias"                        => 0,
				      //                                      "Tot. space"                   => 0,
                                      "Free/total space, Gb"         => 0,
                                      "Name"                         => 0,
                                      "Base URL"                     => 0,
                                      "Type"                         => 0
                                      ),
                  "allusers" => array("0" => "Authorised Grid Users:Active Grid Users",
                                      "help" => "<div align=\"left\">
                                                 <dl>
                                                   <dt><b>Name</b></dt>
                                                   <dd>".$clickable.". ".$str_nam."</dd>
                                                   <dt><b>Affiliation</b></dt>
                                                   <dd>User's affiliation, derived from the personal certificate</dd>
                                                   <dt><b>Jobs</b></dt>
                                                   <dd>Count of all user jobs in the system (running, pending,
                                                     finished or deleted)</dd>
                                                   <dt><b>Sites</b></dt>
                                                   <dd>Shows how many sites authorise this user</dd>
                                                 </dl>
                                                 </div>
",
                                      "#"                            => 0,
                                      "Name"                         => 0,
                                      "Affiliaton"                   => 0,
                                      "Jobs"                         => 0,
                                      "Sites"                        => 0
                                      ),
                  "userres"  => array("0" => "",
                                      "Cluster:queue"                => 0,
                                      "Free CPUs"                    => 0,
                                      "Queued jobs"                  => 0,
                                      "Free disk (MB)"               => 0
                                      ),
                  "ldapdump" => array("0" => "",
                                      "Attribute"                    => 0,
                                      "Value"                        => 0
                                      ),
                  // IS attributes
                  "mdsattr"  => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info valid from (GMT)",
                                      "Mds-validto"   => "Info valid to (GMT)"
                                      ),
                  "isattr"   => array(
                                      "objectClass"   => "objectClass",
                                      "Mds-validfrom" => "Info valid from (GMT)",
                                      "Mds-validto"   => "Info valid to (GMT)",
                                      "nordugrid-cluster-name"               => "Front-end domain name",
                                      "nordugrid-cluster-aliasname"          => "Cluster alias",
                                      "nordugrid-cluster-contactstring"      => "Contact string",
                                      "nordugrid-cluster-interactive-contactstring" => "Interactive contact",
                                      "nordugrid-cluster-comment"            => "Comment",
                                      "nordugrid-cluster-support"            => "E-mail contact",
				      "nordugrid-cluster-acl"                => "Authorised VOs",
                                      "nordugrid-cluster-lrms-type"          => "LRMS type",
                                      "nordugrid-cluster-lrms-version"       => "LRMS version",
                                      "nordugrid-cluster-lrms-config"        => "LRMS details",
                                      "nordugrid-cluster-architecture"       => "Architecture",
                                      "nordugrid-cluster-opsys"              => "Operating system",
                                      "nordugrid-cluster-homogeneity"        => "Homogeneous cluster",
                                      "nordugrid-cluster-nodecpu"            => "CPU type (slowest)",
                                      "nordugrid-cluster-nodememory"         => "Memory (MB, smallest)",
                                      "nordugrid-cluster-totalcpus"          => "CPUs, total",
                                      "nordugrid-cluster-cpudistribution"    => "CPU:machines",
                                      "nordugrid-cluster-benchmark"          => "Benchmark",
                                      "nordugrid-cluster-sessiondir-free"    => "Disk space, available (MB)",
                                      "nordugrid-cluster-sessiondir-total"   => "Disk space, total (MB)",
                                      "nordugrid-cluster-sessiondir-lifetime"=> "Grid session lifetime (min)",
                                      "nordugrid-cluster-cache-free"         => "Cache size, available (MB)",
                                      "nordugrid-cluster-cache-total"        => "Cache size, total (MB)",
                                      "nordugrid-cluster-runtimeenvironment" => "Runtime environment",
                                      "nordugrid-cluster-localse"            => "Storage Element, local",
                                      "nordugrid-cluster-middleware"         => "Grid middleware",
                                      "nordugrid-cluster-totaljobs"          => "Jobs, total amount",
                                      "nordugrid-cluster-usedcpus"           => "CPUs, occupied",
                                      "nordugrid-cluster-queuedjobs"         => "Jobs, queued",
				      "nordugrid-cluster-prelrmsqueued"      => "Grid jobs, awaiting submission",
                                      "nordugrid-cluster-location"           => "Postal code",
                                      "nordugrid-cluster-owner"              => "Owner",
                                      "nordugrid-cluster-issuerca"           => "Certificate issuer",
                                      "nordugrid-cluster-issuerca-hash"      => "Certificate issuer's hash",
                                      "nordugrid-cluster-trustedca"          => "Trusted certificate issuers",
                                      "nordugrid-cluster-nodeaccess"         => "Node IP connectivity",
                                      "nordugrid-cluster-gridarea"           => "Session area (OBSOLETE)",
                                      "nordugrid-cluster-gridspace"          => "Grid disk space (OBSOLETE)",
                                      "nordugrid-cluster-opsysdistribution"  => "OS distribution (OBSOLETE)",
                                      "nordugrid-cluster-runningjobs"        => "Jobs, running (OBSOLETE)",
				      "nordugrid-cluster-credentialexpirationtime" => "Credential expiration time",
                                      "nordugrid-queue-name"                 => "Queue name",
                                      "nordugrid-queue-comment"              => "Comment",
                                      "nordugrid-queue-status"               => "Queue status",
                                      "nordugrid-queue-running"              => "Total occupied CPUs",
				      "nordugrid-queue-localqueued"          => "Local jobs, queued",
				      "nordugrid-queue-prelrmsqueued"        => "Grid jobs, awaiting submission",
                                      "nordugrid-queue-queued"               => "Jobs, queued (OBSOLETE)",
                                      "nordugrid-queue-maxrunning"           => "Jobs, running (max)",
                                      "nordugrid-queue-maxqueuable"          => "Jobs, queueable (max)",
                                      "nordugrid-queue-maxuserrun"           => "Jobs per Unix user (max)",
                                      "nordugrid-queue-maxcputime"           => "CPU time, max. (minutes)",
                                      "nordugrid-queue-mincputime"           => "CPU time, min. (minutes)",
                                      "nordugrid-queue-defaultcputime"       => "CPU time, default (minutes)",
                                      "nordugrid-queue-maxwalltime"          => "Walltime, max. (minutes)",
                                      "nordugrid-queue-minwalltime"          => "Walltime, min. (minutes)",
                                      "nordugrid-queue-defaultwalltime"      => "Walltime, default (minutes)",
                                      "nordugrid-queue-schedulingpolicy"     => "Scheduling policy",
                                      "nordugrid-queue-totalcpus"            => "CPUs, total",
                                      "nordugrid-queue-nodecpu"              => "CPU type",
                                      "nordugrid-queue-nodememory"           => "Memory (MB)",
                                      "nordugrid-queue-architecture"         => "Architecture",
                                      "nordugrid-queue-opsys"                => "Operating system",
                                      "nordugrid-queue-homogeneity"          => "Homogeneous queue",
                                      "nordugrid-queue-gridrunning"          => "CPUs occupied by Grid jobs",
                                      "nordugrid-queue-gridqueued"           => "Grid jobs, queued",
                                      "nordugrid-queue-benchmark"            => "Benchmark",
                                      "nordugrid-queue-assignedcpunumber"    => "CPUs per queue (OBSOLETE)",
                                      "nordugrid-queue-assignedcputype"      => "CPU type (OBSOLETE)",
                                      "nordugrid-job-globalid"               => "ID",
                                      "nordugrid-job-globalowner"            => "Owner",
                                      "nordugrid-job-execcluster"            => "Execution cluster",
                                      "nordugrid-job-execqueue"              => "Execution queue",
                                      "nordugrid-job-stdout"                 => "Standard output file",
                                      "nordugrid-job-stderr"                 => "Standard error file",
                                      "nordugrid-job-stdin"                  => "Standard input file",
                                      "nordugrid-job-reqcputime"             => "Requested CPU time",
                                      "nordugrid-job-reqwalltime"            => "Requested wall clock time",
                                      "nordugrid-job-status"                 => "Status",
                                      "nordugrid-job-queuerank"              => "Position in the queue",
                                      "nordugrid-job-comment"                => "LRMS comment",
                                      "nordugrid-job-submissionui"           => "Submission machine",
                                      "nordugrid-job-submissiontime"         => "Submission time (GMT)",
                                      "nordugrid-job-usedcputime"            => "Used CPU time",
                                      "nordugrid-job-usedwalltime"           => "Used wall clock time",
                                      "nordugrid-job-completiontime"         => "Job completion time (GMT)",
                                      "nordugrid-job-sessiondirerasetime"    => "Erase time (GMT)",
                                      "nordugrid-job-proxyexpirationtime"    => "Proxy expiration time (GMT)",
                                      "nordugrid-job-usedmem"                => "Used memory (KB)",
                                      "nordugrid-job-errors"                 => "Errors",
                                      "nordugrid-job-exitcode"               => "Exit code",
                                      "nordugrid-job-jobname"                => "Name",
                                      "nordugrid-job-runtimeenvironment"     => "Runtime environment",
                                      "nordugrid-job-cpucount"               => "Requested CPUs",
                                      "nordugrid-job-executionnodes"         => "Execution nodes",
                                      "nordugrid-job-gmlog"                  => "GM log file",
                                      "nordugrid-job-clientsoftware"         => "Client version",
                                      "nordugrid-job-rerunable"              => "Rerunnable",
                                      "nordugrid-job-reqcput"                => "Requested time (OBSOLETE)",
                                      "nordugrid-job-gridlog"                => "Gridlog file (OBSOLETE)",
                                      "nordugrid-job-lrmscomment"            => "LRMS comment (OBSOLETE)",
                                      "nordugrid-authuser-name"              => "Name",
                                      "nordugrid-authuser-sn"                => "Subject Name",
                                      "nordugrid-authuser-freecpus"          => "Free CPUs",
                                      "nordugrid-authuser-diskspace"         => "Free disk space (MB)",
                                      "nordugrid-authuser-queuelength"       => "User's queued jobs",
                                      "nordugrid-se-name"                    => "Name",
                                      "nordugrid-se-aliasname"               => "Storage element alias",
                                      "nordugrid-se-type"                    => "Storage element type",
				      "nordugrid-se-acl"                     => "Authorised VOs",
                                      "nordugrid-se-freespace"               => "Free space (MB)",
                                      "nordugrid-se-totalspace"              => "Total space (MB)",
                                      "nordugrid-se-url"                     => "Contact URL",
                                      "nordugrid-se-baseurl"                 => "Contact URL (OBSOLETE)",
                                      "nordugrid-se-accesscontrol"           => "Access control",
                                      "nordugrid-se-authuser"                => "Authorised user (DN)",
                                      "nordugrid-se-location"                => "Postal code",
                                      "nordugrid-se-owner"                   => "Owner",
                                      "nordugrid-se-middleware"              => "Middleware",
                                      "nordugrid-se-issuerca"                => "Certificate issuer",
                                      "nordugrid-se-issuerca-hash"           => "Certificate issuer's hash",
                                      "nordugrid-se-trustedca"               => "Trusted certificate issuers",
                                      "nordugrid-se-comment"                 => "Comment",
                                      "nordugrid-rc-name"                    => "Domain name",
                                      "nordugrid-rc-aliasname"               => "Replica Catalog alias",
                                      "nordugrid-rc-baseurl"                 => "Contact URL",
                                      "nordugrid-rc-authuser"                => "Authorised user (DN)",
                                      "nordugrid-rc-location"                => "Postal code",
                                      "nordugrid-rc-owner"                   => "Owner",
                                      "nordugrid-rc-issuerca"                => "Certificate issuer"
                                      ),
                  // Errors, warnings etc
                  "errors"   => array(
                                      // failure notices
                                      "1"   => "Can not read top-level resource indices",
                                      "2"   => "None of the local indices returned connection",
                                      "3"   => " bad configuration or request timed out",
                                      "4"   => "No Grid jobs found",
                                      "5"   => "No information found",
                                      "6"   => "Server unavailable",
                                      "7"   => " - refresh later",
                                      "8"   => "No queue information found",
                                      "9"   => "No entries found",
                                      "10"  => "No users found",
                                      "11"  => "Not authorised at host",
                                      "12"  => "does not answer",
                                      "13"  => "No recent jobs found for ",
                                      // debug messages
                                      "101" => " Monitor timeouts for GRIS: ",
                                      "102" => " sec on connection and ",
                                      "103" => " sec on search",
                                      "104" => " sec spent searching",
                                      "105" => "Showing resources only in ",
                                      "106" => "Polled top-level indices: ",
                                      "107" => "Got geographical locations, scanned sites: ",
                                      "108" => " sites arranged by geographical location",
                                      "109" => "Search for cluster attributes",
                                      "110" => "Search for queue attributes",
                                      "111" => "No data from ",
                                      "112" => " is up in ",
                                      "113" => " has no resources to offer",
                                      "114" => " Monitor timeouts for GIIS: ",
                                      "115" => "Skipping GRIS: ",
                                      "116" => "not a ",
                                      "117" => "Checking connection: ",
                                      "118" => "OK",
                                      "119" => "That far, detected resources of kind ",
                                      "120" => "LDAP error searching ",
                                      "121" => " status at ",
                                      "122" => "Blacklisted: ",
                                      "123" => "Registrant found for ",
                                      "124" => "Search for SE attributes",
                                      "125" => "Search for users",
                                      "126" => "Search for jobs",
                                      "127" => " has job ",
                                      "128" => " while not being authorized",
                                      "129" => "Can not get object data: error ",
                                      "130" => " Monitor timeouts for EMIR: ",
					  				  "131" => " Monitor timeouts for ARCHERY depends on OS DNS resolver settings (In DNS cache we trust!)",
                                      "132" => "Failed to query the following ARCHERY endpoint: ",
                                      "133" => "Reached the recursive loop limit while querying ARCHERY endpoint: ",
                                      // icon titles
                                      "301" => "Refresh",
                                      "302" => "Print",
                                      "303" => "Help",
                                      "304" => "Close",
                                      "305" => "Red",
                                      "306" => "Grey",
                                      "307" => "All users",
                                      "308" => "Active users",
                                      "309" => "Search",
                                      "310" => "Storage",
                                      "311" => "VOs",
                                      "312" => "Flag of ",
                                      "313" => " Grid processes and ",
                                      "314" => " local processes",
                                      // auxilliary strings
                                      "401" => "Processes",
                                      "402" => "Grid",
                                      "403" => "Local",
                                      "404" => "World",
                                      "405" => "TOTAL",
                                      "406" => " sites",
                                      "407" => "a lot of",
                                      "408" => " GB",
                                      "409" => " ALL",
                                      "410" => "Cluster",
                                      "411" => "Queue",
                                      "412" => "Job",
                                      "413" => "User",
                                      "414" => "Storage",
                                      "415" => "Replica Cat.",
                                      "416" => "Define attributes to display for the object: ",
                                      "417" => "AND of all the expressions will be matched",
                                      "418" => "Leave the righmost field empty to show everything",
                                      "419" => "Display resources or objects of your choice",
                                      "420" => "Distinguished name",
                                      "421" => "Can use a total of ",
                                      "422" => " sites",
                                      "423" => "Resource / object:",
                                      "424" => "Nr.of attributes (def. 6):",
                                      "425" => "Object",
                                      "426" => "Next",
                                      "427" => "Select one",
                                      "428" => "Reset",
                                      "429" => "SHOW"
                                      ),
                  // Post code conversion: only for [en]!
                  "tlconvert" => array (
                                        "AU"  => "Australia",
                                        "AT"  => "Austria",
                                        "AM"  => "Armenia",
                                        "DZ"  => "Algeria",
                                        "BE"  => "Belgium",
                                        "BG"  => "Bulgaria",
                                        "CA"  => "Canada",
                                        "CL"  => "Chile",
                                        "CN"  => "China",
                                        "CZ"  => "Czechia",
                                        "DK"  => "Denmark",
                                        "EE"  => "Estonia",
                                        "FI"  => "Finland",
                                        "FIN" => "Finland",
                                        "SF"  => "Finland",
                                        "FR"  => "France",
                                        "GE"  => "Georgia",
                                        "DE"  => "Germany",
                                        "D"   => "Germany",
                                        "GR"  => "Greece",
                                        "HK"  => "Hong Kong",
                                        "HU"  => "Hungary",
                                        "IS"  => "Iceland",
                                        "IR"  => "Ireland",
                                        "IE"  => "Ireland",
                                        "IT"  => "Italy",
                                        "JP"  => "Japan",
                                        "KEK" => "Japan",
                                        "TOKYO" => "Japan",
                                        "LV"  => "Latvia",
                                        "LT"  => "Lithuania",
                                        "MA"  => "Morocco",
                                        "NL"  => "Netherlands",
                                        "NO"  => "Norway",
                                        "N"   => "Norway",
                                        "PL"  => "Poland",
                                        "PT"  => "Portugal",
                                        "RO"  => "Romania",
                                        "RU"  => "Russia",
                                        "SU"  => "Russia",
                                        "LK"  => "SriLanka",
                                        "SE"  => "Sweden",
                                        "SK"  => "Slovakia",
                                        "SI"  => "Slovenia",
                                        "ES"  => "Spain",
                                        "CH"  => "Switzerland",
                                        "TW"  => "Taiwan",
                                        "TR"  => "Turkey",
                                        "UK"  => "UK",
                                        "UA"  => "Ukraine",
                                        "COM" => "USA",
                                        "GOV" => "USA",
                                        "USA" => "USA",
                                        "US"  => "USA",
                                        "RG"  => "World"
                                        )
                  );

?>
