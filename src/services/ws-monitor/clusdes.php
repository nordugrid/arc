<?php

// Author: oxana.smirnova@hep.lu.se
/**
 * Retrieves cluster or storage information from a given domain
 * Uses LDAP functions of PHP
 * 
 * Author: O.Smirnova (May 2002)
 * inspired by the LDAPExplorer by T.Miao
 * 
 * output:
 * an HTML table, containing the full resource info
 */
include 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('comfun.inc');
require_once('toreload.inc');

// getting parameters

$host   = $_GET["host"];
$port   = $_GET["port"];
$debug  = $_GET["debug"];
$isse   = $_GET["isse"];
if ( !$host )  $host = "quark.hep.lu.se";
if ( !$port )  $port = 2135;
if ( !$debug ) $debug = 0; 
if ( !$isse )  $isse = 0; 

// Setting up the page itself

$toppage  = new LmDoc("clusdes",$host);
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$cert     = &$toppage->cert;

// Header table

$toppage->tabletop("","<b>".$toptitle." <i>$host</i></b>");

// Array defining the attributes to be returned

$qlim = array( QUE_NAME, QUE_QUED, QUE_GQUE, QUE_PQUE, QUE_LQUE, QUE_RUNG, QUE_GRUN, 
               QUE_ASCP, QUE_MAXT, QUE_MINT, QUE_STAT );

$dn      = DN_LOCAL;

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 15;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// establish connection to the requested LDAP server

$chost = $host;
if ( $isse ) $chost=substr(strstr($host,":"),1);

$type = 'DB';
$isis = new $type($cert,array());

if ($isis) {

  // If contact OK, search for clusters

  $ts1 = time();
//  for($i=0; $i<30; $i++)
  $info = $isis->cluster_info($host, $debug);
  $isis->xml_nice_dump($info->Domains->AdminDomain->Services->ComputingService, "cluster", $debug);
  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["109"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
  echo "<br>";

  if (true) {

    // If search returned, check that there are valid entries

    if ($host) {

      // If there are valid entries, tabulate results

      $qentries = $info;
      $queues = $qentries->Domains->AdminDomain->Services->ComputingService->ComputingShares->ComputingShare;
      $queues_new = $qentries->Domains->AdminDomain->Services->ComputingService->ComputingShare;

      $queues = @($queues) ? $queues : $queues_new ;
      $nqueues  = count($queues);

      // HTML table initialisation

      $qtable = new LmTableSp($module,$toppage->$module);

      // loop on the rest of attributes

      define("CMPKEY",QUE_MAXT);
      usort($qentries,"quetcmp");
      for ($k=0; $k<$nqueues; $k++) {
        $qname   =  $queues[$k]->{QUE_NAME};
        $qstatus =  $queues[$k]->{QUE_STAT};
        $qid =  $queues[$k]->ID;
        $queued  = @($queues[$k]->{QUE_QUED}) ? ($queues[$k]->{QUE_QUED}) : 0; /* deprecated since 0.5.38 */
        $locque  = @($queues[$k]->{QUE_LQUE}) ? ($queues[$k]->{QUE_LQUE}) : 0; /* new since 0.5.38 */
        $run     = @($queues[$k]->{QUE_RUNG}) ? ($queues[$k]->{QUE_RUNG}) : 0;
        $cpumin  = @($queues[$k]->{QUE_MINT}) ? $queues[$k]->{QUE_MINT} : "0";
        $cpumax  = @($queues[$k]->{QUE_MAXT}) ? $queues[$k]->{QUE_MAXT} : $queues[$k]->MaxCPUTime; //"&gt;";
        $cpu     = @($queues[$k]->{QUE_ASCP}) ? $queues[$k]->{QUE_ASCP} : "N/A";
        $gridque = @($queues[$k]->{QUE_GQUE} && $queues[$k]->{QUE_LQUE}) ? ($queues[$k]->{QUE_GQUE} - $queues[$k]->{QUE_LQUE}) : 0;
        $gmque   = @($queues[$k]->{QUE_PQUE}) ? ($queues[$k]->{QUE_PQUE}) : 0; /* new since 0.5.38 */
        $gridrun = @($queues[$k]->{QUE_RUNG} && $queues[$k]->LocalRunningJobs) ? ($queues[$k]->{QUE_RUNG} - $queues[$k]->LocalRunningJobs) : 0;
        $quewin  = popup("quelist.php?host=$host&port=$port&qname=$qname&qid=$qid",750,430,6);

        $gridque = $gridque + $gmque;
        if ( $queued == 0 ) $queued = $locque + $gridque;

        // filling the table

        $qrowcont[] = "<a href=\"$quewin\">$qname</a>";
        $qrowcont[] = "$qstatus";
        $qrowcont[] = "$cpumin &ndash; $cpumax";
        $qrowcont[] = "$cpu";
        $qrowcont[] = "$run (".$errors["402"].": $gridrun)";
        $qrowcont[] = "$queued (".$errors["402"].": $gridque)";
        $qtable->addrow($qrowcont);
        $qrowcont = array ();
      }
      $qtable->close();
    }
    else {
      $errno = 8;
      echo "<br><font color=\"red\"><b>".$errors["8"]."</b></font>\n";
      $toppage->close();
      return $errno;
    }
  }
  elseif ( !$isse ) {
    $errno = 5;
    echo "<br><font color=\"red\"><b>".$errors["5"]."</b></font>\n";
    $toppage->close();
    return $errno;
  }
  $toppage->close();
  return 0;
}
else {
  $errno = 6;
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

// Done


?>
