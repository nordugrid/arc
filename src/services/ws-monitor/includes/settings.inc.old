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

// base DNs for searches: local (GRIS), global (GIIS), VO
if ( !defined("DN_LOCAL") ) define("DN_LOCAL","mds-vo-name=local,o=grid");
if ( !defined("DN_GLOBL") ) define("DN_GLOBL","mds-vo-name=NorduGrid,o=grid");
if ( !defined("DN_VIORG") ) define("DN_VIORG","dc=nordugrid,dc=org");
if ( !defined("DN_PEOPL") ) define("DN_PEOPL","ou=people,dc=nordugrid,dc=org");
if ( !defined("DN_GUEST") ) define("DN_GUEST","ou=guests,dc=nordugrid,dc=org");
if ( !defined("DN_TUTOR") ) define("DN_TUTOR","ou=tutorial,dc=nordugrid,dc=org");
if ( !defined("DN_SERVS") ) define("DN_SERVS","ou=services,dc=nordugrid,dc=org");
if ( !defined("DN_RECAT") ) define("DN_RECAT","rc=NorduGrid,dc=nordugrid,dc=org");

// Information system classes and attributes namespace prefix, "nordugrid"
if ( !defined("IS_PREFX") ) define("IS_PREFX","nordugrid");

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
if ( !defined("OBJ_PERS") ) define("OBJ_PERS","organizationalPerson");
if ( !defined("OBJ_RCOL") ) define("OBJ_RCOL","GlobusReplicaLogicalCollection");  /* RC Logical Collection object */
if ( !defined("OBJ_RFIL") ) define("OBJ_RFIL","GlobusReplicaLogicalFile");        /* RC Logical File object */
if ( !defined("OBJ_RFIN") ) define("OBJ_RFIN","GlobusReplicaFileInfo");           /* RC File Info object */
if ( !defined("OBJ_RSEL") ) define("OBJ_RSEL","GlobusReplicaInfo");               /* RC Info object */

// attributes
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

