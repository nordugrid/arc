<?php

// Author: oxana.smirnova@hep.lu.se
/**
 * Retrieves full user information from the NorduGrid
 * Uses LDAP functions of PHP
 *
 * Author: O.Smirnova (June 2002)
 * inspired by the LDAPExplorer by T.Miao
 *
 * input:
 * bdn (base DN, def. mds-vo-name=NordiGrid,o=Grid)
 * owner (DN of a user)
 *
 * output:
 * an HTML table, containing:
 * per each cluster:
 * - list of jobs
 */
include_once 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');

/**
 * @return int
 * @param cpustring string
 * @desc Extract total number of curently free CPUs for a user: USELESS!!!
 */
function freeproc ($cpustring) {
  $ncpu = 0;
  $gcpu = array ();
  // $cpustring looks like "l n:t1 m:t2 ..."
  $gcpu = explode(" ",$cpustring);                // array("l","n:t1","m:t2")
  foreach ( $gcpu as $tcpu ) {
    $curcpu = $tcpu;                              // l or n:t1
    $tpos   = strpos($tcpu,":");                  // 0 or 1
    if ( $tpos ) $curcpu = substr($tcpu,0,$tpos); // n
    $ncpu += $curcpu;
  }

  return $ncpu;
}

// getting parameters

$owner = $_GET["owner"];
$bdn   = DN_GLOBL;
$debug = ( $_GET["debug"] )  ? $_GET["debug"]  : 0;

// Exrracting names

$uname  = rawurldecode($owner);
$family = cnvname($uname, 2);
$uname  = addslashes($uname); // In case $uname contains escape characters

// Setting up the page itself

$toppage  = new LmDoc("userlist",$family);
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$isislist = &$toppage->isislist;
$cert     = &$toppage->cert;

// Header table

$toppage->tabletop("",$toptitle." $family");

// Array defining the attributes to be returned

$lim  = array( "dn", USR_USSN, USR_CPUS, USR_QUEU, USR_DISK );
$ulim = array( "dn", JOB_NAME, JOB_EQUE, JOB_ECLU, JOB_GOWN, JOB_SUBM, JOB_STAT, JOB_USET, JOB_ERRS, JOB_CPUS );

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 20;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["114"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

$type = 'DB';
$isis = new $type($cert,$isislist);

$isis->connect($debug);
$gentries = $isis->get_infos();
$nc       = count($gentries);
if ( !$nc ) {
  $errno = "1";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

$dsarray = array ();
$hostsarray = array ();
$hnarray = array ();
$pnarray = array ();
$sitetag = array (); /* a tag to skip duplicated entries */

// Purging cluster entries
$ncc=0;
foreach ( $gentries as $vo) {
    $sitec = count($vo)/2;
    $ncc += $sitec;
    for ( $i = 0; $i < $sitec; $i++ ) {
        array_push($hostsarray,(string)$vo[(string)$vo[$i]]["EPR"]);
    }
}

for ( $k = 0; $k < count($hostsarray); $k++ ) {
  $clport = $gentries[$k]["port"];

  $clhost = $hostsarray[$k];
  $clconn = $isis->cluster_info($clhost,$debug);

  if ( $clconn && !$sitetag[$clhost] ) {
    array_push($dsarray,$clconn);
    array_push($hnarray,$clhost);
    array_push($pnarray,$clport);
    $sitetag[$clhost] = 1; /* filtering tag */
  }
}

$nhosts = count($dsarray);
if ( !$nhosts ) {
  // NO SITES REPLY...
  $errno = "2";
  echo "<BR><font color=\"red\"><B>".$errors[$errno]."</B></font>\n";
  $toppage->close();
  return $errno;
}

// HTML table initialisation
$utable   = new LmTable("userres",$strings["userres"]);
$urowcont = array();
$dnmsg    = "<b>".$errors["420"]."</b>: ".$uname;
$utable->adderror($dnmsg, "#cccccc");

$nauclu = 0;
$goodds = array();
$goodhn = array();
$goodpn = array();

for ( $ids = 0; $ids < $nhosts; $ids++ ) {

  $hn    = $hnarray[$ids];
  $pn    = $pnarray[$ids];
  $dst   = $dsarray[$ids];
  $curl  = popup("clusdes.php?host=$hn&port=$pn",700,620,1);

  if ($dst) {

    $nqueues = count($dst->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->Associations->ComputingShareID);
    if ($nqueues > 0) {

      $nauclu++;
      array_push($goodds,$dst);
      array_push($goodhn,$hn);
      array_push($goodpn,$pn);

      // If there are valid entries, tabulate results

      $allres  = $dst->Domains->AdminDomain->Services->ComputingService;
      $queues = $allres->ComputingShares->ComputingShare;
      $queues_new = $allres->ComputingShare;

      $queues = @($queues) ? $queues : $queues_new ;
 
      $nqueues = count($queues);

      //   define("CMPKEY",USR_CPUS);
      //   usort($allres,"ldap_entry_comp");

      // loop on queues

      for ($j=0; $j<$nqueues; $j++) {
        $ucluster = $hn;
        $uqueue   = $queues[$j]->MappingQueue;

        if ( $debug == 2 ) dbgmsg("$hn -- $ucluster<br>");

        $qurl  = popup("quelist.php?host=$ucluster&port=$pn&qname=$uqueue",750,430,6);
        $curl  = popup("clusdes.php?host=$ucluster&port=$pn",700,620,1);
        // for FreeCPUs
        $computingmanager = $allres->ComputingManager;
        $curtotcpu = @($computingmanager->{CLU_TCPU}) ? $computingmanager->{CLU_TCPU} : $computingmanager->TotalSlots;
        $curusedcpu = @($computingmanager->SlotsUsedByGridJobs && $computingmanager->SlotsUsedByLocalJobs) ? 
                        $computingmanager->SlotsUsedByLocalJobs + $computingmanager->SlotsUsedByGridJobs : 0;
        $fcpu  = $curtotcpu - $curusedcpu;
        $fproc = freeproc($fcpu);
        $fdisk = @($allres[$j][USR_DISK][0]) ? $allres[$j][USR_DISK][0] : "N/A";
        //$exque = $allres[$j][USR_QUEU][0];
        $exque = $queues[$j]->{QUE_GQUE};
        $urowcont[] = "<a href=\"$curl\">$ucluster</a>:<a href=\"$qurl\">$uqueue</a>";
        $urowcont[] = $fcpu;
        $urowcont[] = $exque;
        $urowcont[] = $fdisk;
        $utable->addrow($urowcont);
        $urowcont = array();
      }
    } else {
      $utable->adderror("<b>".$errors["11"]." <a href=\"$curl\">$hn</a></b>");
    }
  } else {
    $utable->adderror("<b><a href=\"$curl\">$hn</a> ".$errors["12"]."</b>");
  }
}

$utable->adderror("<font color=\"#ffffff\"><i><b>".$errors["421"].$nauclu.$errors["422"]."</b></i></font>", "#0099FF");


$utable->close();
echo "<br>\n";

// HTML table initialisation
$jtable  = new LmTable($module,$toppage->$module);
$rowcont = array();
$jcount  = 0;

$nghosts = count($goodds);

for ( $ids = 0; $ids < $nghosts; $ids++ ) {

  $dst = $goodds[$ids];
  $gpn = $goodpn[$ids];
  $ghn = $goodhn[$ids];

  if ($dst) {

    // If search returned, check that there are valid entries

    $allentries  = $dst->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity;
    $nmatch = count($allentries);
    if ($nmatch > 0) {
      // If there are valid entries, tabulate results

      $entries    = $allentries;
      $njobs      = $nmatch;

      define("CMPKEY",JOB_SUBM);
      //usort($entries,"ldap_entry_comp");

      // loop on jobs

      for ($i=0; $i<$njobs; $i++) {
        if ( $owner != $entries[$i]->{JOB_GOWN}) continue;
        $jobdn   = rawurlencode($entries[$i]->{JOB_GOWN});
        $curstat = $entries[$i]->{JOB_STAT};
        $stahead = substr($curstat,10,8);
        if ($stahead=="FINISHED") {
          $ftime   = substr(strrchr($curstat, " "), 1);
          $ftime   = $entries[$i]->{JOB_COMP};
          $ftime   = cnvtime($ftime);
          $curstat = "FINISHED at: ".$ftime;
        }
        $jname    = htmlentities($entries[$i]->{JOB_NAME});
        $jobname  = ($entries[$i]->{JOB_NAME}) ? $jname : "<font color=\"red\">N/A</font>";
        $queue    = ($entries[$i]->{JOB_EQUE}) ? $entries[$i]->{JOB_EQUE} : "N/A";
        $cluster  = $ghn;
        $time     = ($entries[$i]->{JOB_USET}) ? $entries[$i]->{JOB_USET} : "N/A";
        $ncpus    = ($entries[$i]->{JOB_CPUS}) ? $entries[$i]->{JOB_CPUS} : "";
        $error    = ($entries[$i]->{JOB_ERRS});
        if ( $error ) $error = ( preg_match("/user/i",$error) ) ? "X" : "!";
        if ( $debug == 2 ) dbgmsg("$ghn --- $cluster<br>");
        $newwin   = popup("jobstat.php?host=$cluster&port=$gpn&status=$status&jobdn=$jobdn",750,430,4);
        $quewin   = popup("quelist.php?host=$cluster&port=$gpn&qname=$queue",750,430,6);
        $clstring = popup("clusdes.php?host=$cluster&port=$gpn",700,620,1);
        $jcount++;

        // filling the table

        $rowcont[] = "$jcount&nbsp;<b><font color=\"red\">$error</font></b>";
        $rowcont[] = "<a href=\"$newwin\">$jobname</a>";
        $rowcont[] = "$curstat";
        $rowcont[] = "$time";
        $rowcont[] = "<a href=\"$clstring\">$cluster</a>";
        $rowcont[] = "<a href=\"$quewin\">$queue</a>";
        $rowcont[] = "$ncpus";
        $jtable->addrow($rowcont);
        $rowcont = array();
      }
    }
  }
}
if ( !$jcount ) $jtable->adderror("<b>".$errors["13"].$family."</b>");
$jtable->close();

$toppage->close();
return 0;

// Done

$toppage->close();

?>
