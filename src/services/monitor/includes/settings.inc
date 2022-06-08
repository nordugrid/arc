<?php

// Author: oxana.smirnova@hep.lu.se

//******************** Necessary Information System objects and attributes *************************

// list of available central GIISes
$giislist = array(
//*** List of known indexng servers/GIISes is available at http://www.nordugrid.org/NorduGridMDS/index_service.html
//*** Default ARC Community indexing servers/GIISes:
array("host" => "index1.nordugrid.org",
      "port" => "2135",
      "base" => "mds-vo-name=NorduGrid,o=grid",
      "vo"   => "NorduGrid"),
array("host" => "index2.nordugrid.org",
      "port" => "2135",
      "base" => "mds-vo-name=NorduGrid,o=grid",
      "vo"   => "NorduGrid"),
array("host" => "index3.nordugrid.org",
      "port" => "2135",
      "base" => "mds-vo-name=NorduGrid,o=grid",
      "vo"   => "NorduGrid"),
array("host" => "index4.nordugrid.org",
      "port" => "2135",
      "base" => "mds-vo-name=NorduGrid,o=grid",
      "vo"   => "NorduGrid")
//*** A country-level GIIS example, use as many as you wish to monitor:
//,
//array("host" => "f9pc18.ijs.si",
//      "port" => "2135",
//      "base" => "mds-vo-name=Slovenia,o=grid",
//      "vo"   => "Slovenia")
//*** A single site GRIS example, use as many as you wish to monitor:
//,
//array("host" => "gridmaster.pzr.uni-rostock.de",
//      "port" => "2135",
//      "base" => "nordugrid-cluster-name=gridmaster.pzr.uni-rostock.de,mds-vo-name=local,o=grid",
//      "vo"   => "Germany")
);

// list of ARCHERY endpoints to query
$archery_list = array (
    array (
/* ATLAS at NDGF */ 
       "endpoint" => "ndgf.org")
/* NorduGrid */,
    array (
       "endpoint" => "nordugrid.org")
);

// list of available EMIRs
/*
$emirslist = array(
array("schema" => "https",
      "host" => "testbed-emi5.grid.upjs.sk",
      "port" => "54321",
      "base" => "mds-vo-name=NorduGrid,o=grid")
);
/*
 * To set up TLS connection to EMIR server client certificate is mandatory:
 * Convert client certificate from *.pfx (pkcs12) into *.pem with openssl (if needed):
 * > openssl pkcs12 -in keys.pfx -out keys.pem
 */
$cert = "/var/www/monitor-svn/includes/test.pem";
$cert_pass = 'emi';

// base DNs for searches: local (GRIS), global (GIIS), VO
if ( !defined("DN_LOCAL") ) define("DN_LOCAL","mds-vo-name=local,o=grid");
if ( !defined("DN_GLUE") )  define("DN_GLUE","o=glue");
if ( !defined("DN_GLOBL") ) define("DN_GLOBL","mds-vo-name=NorduGrid,o=grid");
if ( !defined("DN_VIORG") ) define("DN_VIORG","dc=nordugrid,dc=org");
if ( !defined("DN_PEOPL") ) define("DN_PEOPL","ou=people,dc=nordugrid,dc=org");
if ( !defined("DN_GUEST") ) define("DN_GUEST","ou=guests,dc=nordugrid,dc=org");
if ( !defined("DN_TUTOR") ) define("DN_TUTOR","ou=tutorial,dc=nordugrid,dc=org");
if ( !defined("DN_SERVS") ) define("DN_SERVS","ou=services,dc=nordugrid,dc=org");
if ( !defined("DN_RECAT") ) define("DN_RECAT","rc=NorduGrid,dc=nordugrid,dc=org");

// Information system classes and attributes namespace prefix, for NorduGRID schema, "nordugrid"
if ( !defined("IS_PREFX") ) define("IS_PREFX","nordugrid");
// Information system classes and attributes namespace prefix for GLUE2 schema, "GLUE2". Due to 
// the way the php library works, this needs to be all lowercase.
if ( !defined("IS_PREFXG") ) define("IS_PREFXG","glue2");

// Cache location (use ../htdata when installing directly in ./htdocs)
if ( !defined("CACHE_LOCATION") ) define("CACHE_LOCATION","cache");

// Extra title to be added to "Grid Monitor" (e.g. My Favorite)
if ( !defined("EXTRA_TITLE") ) define("EXTRA_TITLE","");

//=========================================================================
// =================== no need to change things below =====================
//=========================================================================

// objectclasses
if ( !defined("OBJ_CLUS") ) define("OBJ_CLUS",IS_PREFX."-cluster");
if ( !defined("OBJ_STEL") ) define("OBJ_STEL",IS_PREFX."-se");
if ( !defined("OBJ_QUEU") ) define("OBJ_QUEU",IS_PREFX."-queue");
if ( !defined("OBJ_AJOB") ) define("OBJ_AJOB",IS_PREFX."-job");
if ( !defined("OBJ_USER") ) define("OBJ_USER",IS_PREFX."-authuser");

//GLUE2
if ( !defined("GOBJ_CLUS") ) define("GOBJ_CLUS",IS_PREFXG."ComputingService");
// SE are currently not supported in our GLUE2 rendering.
//if ( !defined("GOBJ_STEL") ) define("GOBJ_STEL",IS_PREFXG."-se");
if ( !defined("GOBJ_QUEU") ) define("GOBJ_QUEU",IS_PREFXG."ComputingShare");
if ( !defined("GOBJ_AJOB") ) define("GOBJ_AJOB",IS_PREFXG."ComputingActivity");
// GLUE2 has no equivalent to authuser at the moment.
//if ( !defined("GOBJ_USER") ) define("GOBJ_USER",IS_PREFXG."-authuser");
// Making everything lowercase due to php ldap library
if ( !defined("GOBJ_LOC") ) define("GOBJ_LOC",IS_PREFXG."location");
if ( !defined("GOBJ_MAN") ) define("GOBJ_MAN",IS_PREFXG."manager");
if ( !defined("GOBJ_CON") ) define("GOBJ_CON",IS_PREFXG."contact");
if ( !defined("GOBJ_ADMD") ) define("GOBJ_ADMD",IS_PREFXG."admindomain");
if ( !defined("GOBJ_EENV") ) define("GOBJ_EENV",IS_PREFXG."executionenvironment");
if ( !defined("GOBJ_BENC") ) define("GOBJ_BENC",IS_PREFXG."benchmark");
if ( !defined("GOBJ_AENV") ) define("GOBJ_AENV",IS_PREFXG."applicationenvironment");

if ( !defined("OBJ_PERS") ) define("OBJ_PERS","organizationalPerson");
if ( !defined("OBJ_RCOL") ) define("OBJ_RCOL","GlobusReplicaLogicalCollection");  /* RC Logical Collection object */
if ( !defined("OBJ_RFIL") ) define("OBJ_RFIL","GlobusReplicaLogicalFile");        /* RC Logical File object */
if ( !defined("OBJ_RFIN") ) define("OBJ_RFIN","GlobusReplicaFileInfo");           /* RC File Info object */
if ( !defined("OBJ_RSEL") ) define("OBJ_RSEL","GlobusReplicaInfo");               /* RC Info object */

// attributes
//NG
if ( !defined("CLU_NAME") ) define("CLU_NAME",IS_PREFX."-cluster-name");
if ( !defined("CLU_ANAM") ) define("CLU_ANAM",IS_PREFX."-cluster-aliasname");
if ( !defined("CLU_ZIPC") ) define("CLU_ZIPC",IS_PREFX."-cluster-location");
if ( !defined("CLU_TCPU") ) define("CLU_TCPU",IS_PREFX."-cluster-totalcpus");
if ( !defined("CLU_UCPU") ) define("CLU_UCPU",IS_PREFX."-cluster-usedcpus");
if ( !defined("CLU_TJOB") ) define("CLU_TJOB",IS_PREFX."-cluster-totaljobs");
if ( !defined("CLU_QJOB") ) define("CLU_QJOB",IS_PREFX."-cluster-queuedjobs"); /* deprecated since 0.5.38 */
if ( !defined("CLU_OWNR") ) define("CLU_OWNR",IS_PREFX."-cluster-owner");
if ( !defined("CLU_SUPP") ) define("CLU_SUPP",IS_PREFX."-cluster-support");
if ( !defined("CLU_PQUE") ) define("CLU_PQUE",IS_PREFX."-cluster-prelrmsqueued"); /* new since 0.5.38 */
if ( !defined("SEL_NAME") ) define("SEL_NAME",IS_PREFX."-se-name");
if ( !defined("SEL_BURL") ) define("SEL_BURL",IS_PREFX."-se-baseurl"); /* gone since 0.5.26 */
if ( !defined("SEL_CURL") ) define("SEL_CURL",IS_PREFX."-se-url"); /* in place since 0.5.26 */
if ( !defined("SEL_ANAM") ) define("SEL_ANAM",IS_PREFX."-se-aliasname");
if ( !defined("SEL_TYPE") ) define("SEL_TYPE",IS_PREFX."-se-type");
if ( !defined("SEL_FREE") ) define("SEL_FREE",IS_PREFX."-se-freespace");
if ( !defined("SEL_TOTA") ) define("SEL_TOTA",IS_PREFX."-se-totalspace");
if ( !defined("SEL_USER") ) define("SEL_USER",IS_PREFX."-se-authuser");
if ( !defined("QUE_NAME") ) define("QUE_NAME",IS_PREFX."-queue-name");
if ( !defined("QUE_STAT") ) define("QUE_STAT",IS_PREFX."-queue-status");
if ( !defined("QUE_RUNG") ) define("QUE_RUNG",IS_PREFX."-queue-running");
if ( !defined("QUE_GRUN") ) define("QUE_GRUN",IS_PREFX."-queue-gridrunning");
if ( !defined("QUE_MAXR") ) define("QUE_MAXR",IS_PREFX."-queue-maxrunning");
if ( !defined("QUE_QUED") ) define("QUE_QUED",IS_PREFX."-queue-queued"); /* deprecated since 0.5.38 */
if ( !defined("QUE_LQUE") ) define("QUE_LQUE",IS_PREFX."-queue-localqueued"); /* new since 0.5.38 */
if ( !defined("QUE_PQUE") ) define("QUE_PQUE",IS_PREFX."-queue-prelrmsqueued"); /* new since 0.5.38 */
if ( !defined("QUE_GQUE") ) define("QUE_GQUE",IS_PREFX."-queue-gridqueued");
if ( !defined("QUE_MAXQ") ) define("QUE_MAXQ",IS_PREFX."-queue-maxqueuable");
if ( !defined("QUE_ASCP") ) define("QUE_ASCP",IS_PREFX."-queue-totalcpus");
if ( !defined("QUE_MINT") ) define("QUE_MINT",IS_PREFX."-queue-mincputime");
if ( !defined("QUE_MAXT") ) define("QUE_MAXT",IS_PREFX."-queue-maxcputime");
if ( !defined("JOB_GLID") ) define("JOB_GLID",IS_PREFX."-job-globalid");
if ( !defined("JOB_NAME") ) define("JOB_NAME",IS_PREFX."-job-jobname");
if ( !defined("JOB_STAT") ) define("JOB_STAT",IS_PREFX."-job-status");
if ( !defined("JOB_EQUE") ) define("JOB_EQUE",IS_PREFX."-job-execqueue");
if ( !defined("JOB_ECLU") ) define("JOB_ECLU",IS_PREFX."-job-execcluster");
if ( !defined("JOB_GOWN") ) define("JOB_GOWN",IS_PREFX."-job-globalowner");
if ( !defined("JOB_USET") ) define("JOB_USET",IS_PREFX."-job-usedcputime");
if ( !defined("JOB_USEM") ) define("JOB_USEM",IS_PREFX."-job-usedmem");
if ( !defined("JOB_SUBM") ) define("JOB_SUBM",IS_PREFX."-job-submissiontime");
if ( !defined("JOB_COMP") ) define("JOB_COMP",IS_PREFX."-job-completiontime");
if ( !defined("JOB_ERRS") ) define("JOB_ERRS",IS_PREFX."-job-errors");
if ( !defined("JOB_CPUS") ) define("JOB_CPUS",IS_PREFX."-job-cpucount");
if ( !defined("USR_NAME") ) define("USR_NAME",IS_PREFX."-authuser-name");
if ( !defined("USR_USSN") ) define("USR_USSN",IS_PREFX."-authuser-sn");
if ( !defined("USR_CPUS") ) define("USR_CPUS",IS_PREFX."-authuser-freecpus");
if ( !defined("USR_QUEU") ) define("USR_QUEU",IS_PREFX."-authuser-queuelength");
if ( !defined("USR_DISK") ) define("USR_DISK",IS_PREFX."-authuser-diskspace");

//GLUE2. All lowercase due to php ldap library
// in ComputingService
if ( !defined("GCLU_NAME") ) define("GCLU_NAME",IS_PREFXG."entityname");
if ( !defined("GCLU_ANAM") ) define("GCLU_ANAM",IS_PREFXG."entityname");
// this number seems to be incorrect, check infosys code
if ( !defined("GCLU_TJOB") ) define("GCLU_TJOB",IS_PREFXG."computingservicetotaljobs");
// queuedjobs has no equivalent in GLUE2. Must be calculated with some maths. Since it's deprecated, who cares
//if ( !defined("GCLU_QJOB") ) define("GCLU_QJOB",IS_PREFXG."-cluster-queuedjobs"); /* deprecated since 0.5.38 */
if ( !defined("GCLU_PQUE") ) define("GCLU_PQUE",IS_PREFXG."computingserviceprelrmswaitingjobs");
// in Location
if ( !defined("GCLU_ZIPC") ) define("GCLU_ZIPC",IS_PREFXG."locationpostcode");
// in ComputingManager
// TODO: There are discrepancies on how LogicalCPUs are reported.Maybe TotalSlots is better. This needs to be fixed in the infosys.
//if ( !defined("GCLU_TCPU") ) define("GCLU_TCPU",IS_PREFXG."computingmanagertotallogicalcpus");
if ( !defined("GCLU_TCPU") ) define("GCLU_TCPU",IS_PREFXG."computingmanagertotalslots");
// usedcpus is not available in GLUE2 as such. SlotsUsedByGridJobs should be ok, renaming to GCLU_GCPU
if ( !defined("GCLU_GCPU") ) define("GCLU_GCPU",IS_PREFXG."computingmanagerslotsusedbygridjobs");
if ( !defined("GCLU_LCPU") ) define("GCLU_LCPU",IS_PREFXG."computingmanagerslotsusedbylocaljobs");
// in Contact
if ( !defined("GCLU_SUPP") ) define("GCLU_SUPP",IS_PREFXG."contactdetail");
// in AdminDomain 
if ( !defined("GCLU_OWNR") ) define("GCLU_OWNR",IS_PREFXG."admindomainowner");
// Storage element not supported in current GLUE2
//if ( !defined("GSEL_NAME") ) define("GSEL_NAME",IS_PREFXG."-se-name");
//if ( !defined("GSEL_BURL") ) define("GSEL_BURL",IS_PREFXG."-se-baseurl"); /* gone since 0.5.26 */
//if ( !defined("GSEL_CURL") ) define("GSEL_CURL",IS_PREFXG."-se-url"); /* in place since 0.5.26 */
//if ( !defined("GSEL_ANAM") ) define("GSEL_ANAM",IS_PREFXG."-se-aliasname");
//if ( !defined("GSEL_TYPE") ) define("GSEL_TYPE",IS_PREFXG."-se-type");
//if ( !defined("GSEL_FREE") ) define("GSEL_FREE",IS_PREFXG."-se-freespace");
//if ( !defined("GSEL_TOTA") ) define("GSEL_TOTA",IS_PREFXG."-se-totalspace");
//if ( !defined("GSEL_USER") ) define("GSEL_USER",IS_PREFXG."-se-authuser");
// In ComputingShare
if ( !defined("GQUE_NAME") ) define("GQUE_NAME",IS_PREFXG."entityname");
if ( !defined("GQUE_MAPQ") ) define("GQUE_MAPQ",IS_PREFXG."computingsharemappingqueue");
if ( !defined("GQUE_STAT") ) define("GQUE_STAT",IS_PREFXG."computingshareservingstate");
// RUNG is all the running jobs in the queue, NG queue-running, maps to GLUE2 RunningJobs
if ( !defined("GQUE_RUNG") ) define("GQUE_RUNG",IS_PREFXG."computingsharerunningjobs");
// GRUN is the grid jobs only in the queue, NG queue-gridrunning, there is no equivalent for this in GLUE2. Must be calculated from total-local jobs in the queue
//if ( !defined("GQUE_GRUN") ) define("GQUE_GRUN",IS_PREFXG."");
if ( !defined("GQUE_MAXR") ) define("GQUE_MAXR",IS_PREFXG."computingsharemaxrunningjobs");
if ( !defined("GQUE_LQUE") ) define("GQUE_LQUE",IS_PREFXG."computingsharelocalwaitingjobs"); /* new since 0.5.38 */
// LRUN is non-grid jobs running in the queue
if ( !defined("GQUE_LRUN") ) define("GQUE_LRUN",IS_PREFXG."computingsharelocalrunningjobs"); 
if ( !defined("GQUE_PQUE") ) define("GQUE_PQUE",IS_PREFXG."computingshareprelrmswaitingjobs"); /* new since 0.5.38 */
//This is equivalent to the deprecated -queue-queued, /* deprecated since 0.5.38 */, but exists in GLUE2
if ( !defined("GQUE_QUED") ) define("GQUE_QUED",IS_PREFXG."computingsharewaitingjobs");
if ( !defined("GQUE_MAXQ") ) define("GQUE_MAXQ",IS_PREFXG."computingsharemaxtotaljobs");
// No such thing in GLUE2, maybe maxtotaljobs*maxslotsperjob=1 can be used TODO
//if ( !defined("GQUE_ASCP") ) define("GQUE_ASCP",IS_PREFXG."-queue-totalcpus");
if ( !defined("GQUE_MINT") ) define("GQUE_MINT",IS_PREFXG."computingsharemincputime");
if ( !defined("GQUE_MAXT") ) define("GQUE_MAXT",IS_PREFXG."computingsharemaxcputime");
// in ComputingActivity -- these are disabled by default in GLUE2 LDAP.
if ( !defined("GJOB_GLID") ) define("GJOB_GLID",IS_PREFXG."computingactivityidfromendpoint");
if ( !defined("GJOB_NAME") ) define("GJOB_NAME",IS_PREFXG."name");
// Job state is multivalued in GLUE2 (for each state model). Must be handled differently in rendering
if ( !defined("GJOB_STAT") ) define("GJOB_STAT",IS_PREFXG."computingactivitystate");
if ( !defined("GJOB_EQUE") ) define("GJOB_EQUE",IS_PREFXG."computingactivityqueue");
if ( !defined("GJOB_ECLU") ) define("GJOB_ECLU",IS_PREFXG."computingactivityexecutionnode");
if ( !defined("GJOB_GOWN") ) define("GJOB_GOWN",IS_PREFXG."computingactivityowner");
if ( !defined("GJOB_USET") ) define("GJOB_USET",IS_PREFXG."computingactivityusedtotalcputime");
if ( !defined("GJOB_USEM") ) define("GJOB_USEM",IS_PREFXG."computingactivityusedmainmemory");
if ( !defined("GJOB_SUBM") ) define("GJOB_SUBM",IS_PREFXG."computingactivitysubmissiontime");
if ( !defined("GJOB_COMP") ) define("GJOB_COMP",IS_PREFXG."computingactivityendtime");
if ( !defined("GJOB_ERRS") ) define("GJOB_ERRS",IS_PREFXG."computingactivityerror");
if ( !defined("GJOB_CPUS") ) define("GJOB_CPUS",IS_PREFXG."computingactivityrequestedslots");
// UserInfo is not implemented in our GLUE2 and never will. Most likely all this below and the
// relative code can be removed
//if ( !defined("GUSR_NAME") ) define("GUSR_NAME",IS_PREFXG."-authuser-name");
//if ( !defined("GUSR_USSN") ) define("GUSR_USSN",IS_PREFXG."-authuser-sn");
//if ( !defined("GUSR_CPUS") ) define("GUSR_CPUS",IS_PREFXG."-authuser-freecpus");
//if ( !defined("GUSR_QUEU") ) define("GUSR_QUEU",IS_PREFXG."-authuser-queuelength");
//if ( !defined("GUSR_DISK") ) define("GUSR_DISK",IS_PREFXG."-authuser-diskspace");

// This VO information is relative to country info and has nothing to do with the VO-VOMS concept.
// TODO: assess how relevant this can be for GLUE2.
if ( !defined("VO_USCN" ) ) define("VO_USCN" ,"cn");
if ( !defined("VO_USSN" ) ) define("VO_USSN" ,"sn");
if ( !defined("VO_DESC" ) ) define("VO_DESC" ,"description");

if ( !defined("VO_MAIL" ) ) define("VO_MAIL" ,"mail");
if ( !defined("VO_INST" ) ) define("VO_INST" ,"o");

//************************************* Grid Monitor top window style ******************************
$def_loadmon = array(
"refresh" => 120,
"bgcolor" => "#ffffff",
"thcolor" => "#005659",
"lcolor"  => "#005659",
"tbcolor" => "#ffecb5",
"thfont"  => "face=\"sans-serif\" color=#ffffff",
"tbfont"  => "face=\"sans-serif\""
);

//************************************* Cluster description style **********************************
$def_clusdes = array (
//"title"   => $theaders["clusdes"][0],
"refresh" => 600,
"bgcolor" => "#ffcc33",
"thcolor" => "#000099",
"lcolor"  => "#000099",
"tbcolor" => "#ffffcc",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

//*************************************** Job statistics style *************************************
$def_jobstat = array (
"refresh" => 600,
"bgcolor" => "#ffffff",
"thcolor" => "#ffcc33",
"lcolor"  => "#000099",
"tbcolor" => "#ffffcc",
"thfont"  => "face=\"sans-serif\" color=\"#000000\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

//******************************************* VO list style ***************************************
$def_volist = array(
"refresh" => 0,
"bgcolor" => "#ffffff",
"thcolor" => "#ffcc33",
"lcolor"  => "#ffff00",
"tbcolor" => "#cc0033",
"thfont"  => "face=\"sans-serif\" color=\"#993300\"",
"tbfont"  => "face=\"sans-serif\" color=\"#ffffff\""
);

//***************************************** VO user base style *************************************
$def_vousers = array(
"refresh" => 0,
"bgcolor" => "#ffffff",
"thcolor" => "#ffcc33",
"lcolor"  => "#ffcccc",
"tbcolor" => "#000099",
"thfont"  => "face=\"sans-serif\" color=\"#000000\"",
"tbfont"  => "face=\"sans-serif\" color=\"#ffffff\""
);

//***************************************** User job list style ************************************
$def_userlist = array(
"refresh" => 0,
"bgcolor" => "#ffffcc",
"thcolor" => "#ffcc33",
"lcolor"  => "#000099",
"tbcolor" => "#ffffff",
"thfont"  => "face=\"sans-serif\" color=\"#000000\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

$def_userres = array(
"thcolor" => "#000099",
"tbcolor" => "#ffffcc",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

//**************************************** Attribute list style ************************************
$def_attlist = array(
"refresh" => 0,
"bgcolor" => "#ffffff",
"thcolor" => "#000099",
"lcolor"  => "#000099",
"tbcolor" => "#ccffff",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

//****************************************** Queue job list style **********************************
$def_quelist = array(
"refresh" => 300,
"bgcolor" => "#ffffff",
"thcolor" => "#000099",
"lcolor"  => "#000099",
"tbcolor" => "#ffffcc",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

//******************************************* SE info style ***************************************
$def_sestat = array(
"refresh" => 300,
"bgcolor" => "#ffffff",
"thcolor" => "#ffcc33",
"lcolor"  => "#003300",
"tbcolor" => "#CCCC99",
"thfont"  => "face=\"sans-serif\" color=\"#990000\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000000\""
);

//******************************************* Users info style ***************************************
$def_allusers = array(
"refresh" => 0,
"bgcolor" => "#ffffff",
"thcolor" => "#339966",
"lcolor"  => "#003300",
"tbcolor" => "#ccffcc",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000000\""
);

//***************************** LDAP parameters dump style - no need to modify *********************
$def_ldapdump = array(
"thcolor" => "#000099",
"tbcolor" => "#ffffcc",
"thfont"  => "face=\"sans-serif\" color=\"#ffffff\"",
"tbfont"  => "face=\"sans-serif\" color=\"#000099\""
);

?>

