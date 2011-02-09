<?php

// Author: oxana.smirnova@hep.lu.se
/**
 * Retrieves the queue job list information from a given NorduGrid domain
 * Uses LDAP functions of PHP
 *
 * Author: O.Smirnova (June 2002)
 * inspired by the LDAPExplorer by T.Miao
 *
 * input:
 * host (default: grid.quark.lu.se)
 * port (default: 2135)
 * qname (default: "default")
 *
 * output:
 * an HTML table, containing the list of the jobs in the queue
 */
include_once 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('toreload.inc');

// getting parameters

$host   = ( $_GET["host"] )  ? $_GET["host"]  : "quark.hep.lu.se";
$port   = ( $_GET["port"] )  ? $_GET["port"]  : 2135;
$qname  = $_GET["qname"];
$qid    = $_GET["qid"];
$debug  = ( $_GET["debug"] ) ? $_GET["debug"] : 0;

// Setting up the page itself

$toppage  = new LmDoc("quelist",$qname." (".$host.")");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$cert     = &$toppage->cert;

$clstring = popup("clusdes.php?host=$host&port=$port",700,620,1);

// Header table

$toppage->tabletop("","<b><i>".$toptitle." ".$qname." (<a href=\"$clstring\">".$host."</a>)</i></b>");

$lim = array( "dn", JOB_NAME, JOB_GOWN, JOB_SUBM, JOB_STAT, JOB_COMP, JOB_USET, JOB_USEM, JOB_ERRS, JOB_CPUS, JOB_EQUE );

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 15;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// ldapsearch filter strings for cluster and queues

$filstr = "(objectclass=".OBJ_AJOB.")";
$dn     = DN_LOCAL;
$topdn  = DN_GLOBL;

// Establish connection to the requested information server

$type = 'DB';
$isis = new $type($cert,array());
if ($isis) {

  // If contact OK, search for NorduGrid clusters

  if ($host) {
    $ts1 = time();
    $entries =  $isis->cluster_info($host);
    $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["110"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
    if ($entries) {
      $queues = $entries->Domains->AdminDomain->Services->ComputingService->ComputingShares->ComputingShare;
      $queues_new = $entries->Domains->AdminDomain->Services->ComputingService->ComputingShare;

      $queues = @($queues) ? $queues : $queues_new ;
 
      $actual_queue = NULL;
      foreach ($queues as $queue) {
        if ($queue->ID == $qid) $actual_queue = $queue;
      }

      $isis->xml_nice_dump($actual_queue, "queue", $debug);
      echo "<br>";

      $jobs = $entries->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity;
      $njobs  = count($jobs);

      define("CMPKEY",JOB_SUBM);
      usort($entries,"ldap_entry_comp");

      // HTML table initialisation

      $ltable = new LmTable($module,$toppage->$module);

      // loop on jobs

      $nj = 0;
      for ($i=0; $i<$njobs; $i++) {
        $equeue  = (string)$jobs[$i]->{JOB_EQUE};
        if ( $equeue !== $qname ) {
          if ( $debug == 2 ) dbgmsg(",".$equeue." != ".$qname.",");
          continue;
        }
        $jobdn   = rawurlencode($jobs[$i]->ID);
        $curstat = $jobs[$i]->{JOB_STAT};
        $stahead = substr($curstat,0,12);
        $ftime = "";
        if ($stahead=="FINISHED at:") {
          $ftime = substr(strrchr($curstat, " "), 1);
        } elseif ($curstat=="FINISHED") {
          $ftime = $jobs[$i]->{JOB_COMP};
        }
        if ( $ftime ) {
          $ftime   = cnvtime($ftime);
          $curstat = "FINISHED at: ".$ftime;
        }
        $uname    = $jobs[$i]->{JOB_GOWN};
        $encuname = rawurlencode($uname);
        $family   = cnvname($uname, 2);

        $jname   = htmlentities($jobs[$i]->{JOB_NAME});
        $jobname = ($jobs[$i]->{JOB_NAME}) ? $jname : "<font color=\"red\">N/A</font>";
        $time    = ($jobs[$i]->{JOB_USET}) ? $jobs[$i]->{JOB_USET} : "";
        $memory  = ($jobs[$i]->{JOB_USEM}) ? $jobs[$i]->{JOB_USEM} : "";
        $ncpus   = ($jobs[$i]->{JOB_CPUS}) ? $jobs[$i]->{JOB_CPUS} : "";
        $error   = ($jobs[$i]->{JOB_ERRS});
        if ( $error ) $error = ( preg_match("/user/i",$error) ) ? "X" : "!";
        $status  = "All";
        $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jobdn",750,430,4);
        $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname",700,500,5);

        // filling the table

        $nj++;
        $lrowcont[] = "$nj&nbsp;<b><font color=\"red\">$error</font></b>";
        $lrowcont[] = "<a href=\"$newwin\">$jobname</a>";
        $lrowcont[] = "<a href=\"$usrwin\">$family</a>";
        $lrowcont[] = "$curstat";
        $lrowcont[] = "$time";
        $lrowcont[] = "$memory";
        $lrowcont[] = "$ncpus";
        $ltable->addrow($lrowcont);
        $lrowcont = array ();
      }
      $ltable->close();
    } else {
      $errno = "4";
      echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
      $toppage->close();
      return $errno;
    }
  } else {
    $errno = "5";
    echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
    $toppage->close();
    return $errno;
  }
  $toppage->close();
  return 0;
}
else {
  $errno = "6";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

// Done

$toppage->close();

?>
