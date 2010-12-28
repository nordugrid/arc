<?php
 
// Author: oxana.smirnova@hep.lu.se

/* ALL GRID USERS LISTING */
/**
 * Retrieves the list of authorised users and their jobs
 * Uses SOAPClient functions of PHP
 *
 * Author: O.Smirnova (April 2004)
 *
 * input:
 * debug (debug level)
 * limit (minimal nr. of jobs filter)
 *
 * output:
 * an HTML table, containing:
 * a list of all authorised users and job count
 */

include_once 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');

// getting parameters

$debug  = $_GET["debug"];
$jobnum = $_GET["limit"];
if ( !$jobnum ) $jobnum = 0;
if ( !$debug ) $debug = 0; 

// Setting up the page itself

$toppage  = new LmDoc("allusers");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$isislist = &$toppage->isislist;
$cert     = &$toppage->cert;
$yazyk    = &$toppage->language;

// Array defining the attributes to be returned

$lim  = array( "dn", USR_USSN ); /* need only SN per each user */
$jlim = array( "dn", JOB_GOWN ); /* Job owner only is needed */

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 20;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["114"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// Header table

$titles = explode(":",$toptitle); // two alternative titles, separated by column
$gtitle = $titles[0];
if ( $jobnum ) $gtitle = $titles[1];
$toppage->tabletop($gtitle,"");

$family = cnvname($ussn);

// Search all clusters for users

$ts1  = time();
$type = 'DB';
$isis = new $type($cert,$isislist);

$isis->connect($debug);
$gentries = $isis->get_infos();
$ts2  = time(); if($debug) dbgmsg("<br><b>".$errors["125"]." (".($ts2-$ts1).$errors["104"].")</b><br>");

$nc   = count($gentries);
if ( !$nc ) {
  // NO SITES FOUND!
  $errno = "1";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

$dsarray = array ();
$hostsarray = array ();
$hnarray = array ();
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


// Get all clusters informations

$jiarray = array();
$ts1     = time();
for ( $k = 0; $k < count($hostsarray); $k++ ) {
  $clhost = $hostsarray[$k];
  $clconn = $isis->cluster_info($clhost,$debug);

  if ( $clconn && !$sitetag[$clhost] ) {
    array_push($dsarray,$clconn);
    array_push($hnarray,$clhost);
    $sitetag[$clhost] = 1; /* filtering tag */
  }
}
$ts2     = time(); if($debug) dbgmsg("<br><b>".$errors["126"]." (".($ts2-$ts1).$errors["104"].")</b><br>");

$nhosts = count($dsarray);
if ( !$nhosts ) {
  // NO SITES REPLY...
  $errno = "2";
  echo "<BR><font color=\"red\"><B>".$errors[$errno]."</B></font>\n";
  $toppage->close();
  return $errno;
}


// Loop on clusters; building user list

$usrlist = array ();

for ( $ids = 0; $ids < $nhosts; $ids++ ) {

  $dst = array (); $dst = $dsarray[$ids];

  if ($dst) {

    $njobs  = count($dst->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity);

    if ($nusers > 0 || $njobs > 0) {

      // If there are valid entries, tabulate results

      $allres  = array();
      $alljobs = array();
      $alljobs = $dst->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity;
      $nusers  = $results["count"];

      // loop on users, filling $usrlist[$ussn]["name"] and counting $usrlist[$ussn]["hosts"]

      for ($j=0; $j<$nusers; $j++) {
        $ussn   = $results[$j][USR_USSN][0];
        $family = cnvname($ussn, 2);
        if ( $family == "host" || strlen($family) < 2 ) continue;

        $ussn   = trim($ussn);
        $ussn   = addslashes($ussn); // In case $ussn contains escape characters

        if ( !$usrlist[$ussn] ) {
          $usrlist[$ussn]["name"]  = $family;
          $usrlist[$ussn]["org"]   = getorg($ussn);
          $usrlist[$ussn]["jobs"]  = 0;
          $usrlist[$ussn]["hosts"] = 0;
        }
        $usrlist[$ussn]["hosts"]++;
      }

      // loop on jobs, filling $usrlist[$jown]["jobs"]

      for ($k=0; $k<$njobs; $k++) {
        $jdn    = $alljobs[$k]->{JOB_GOWN};
        $jown   = $alljobs[$k]->{JOB_GOWN};
        $family = cnvname($jown, 2);
        if ( $family == "host" || strlen($family) < 2 ) continue;

        $jown   = addslashes($jown); // In case $jown contains escape characters

        if ( !$usrlist[$jown] ) {
          // Shouldn't be happening, but...
          $usrlist[$jown]["name"] = $family;
          $usrlist[$jown]["org"]  = getorg($jown);
          $usrlist[$jown]["jobs"] = 0;
          if( $debug == 2 ) dbgmsg("<font color=\"red\">$family".$errors["127"]."$jdn".$errors["128"]."</font><br>");
        }
        $usrlist[$jown]["jobs"]++;
      }
    }
  }
}

uasort($usrlist,"hncmp");

// HTML table initialisation
$utable   = new LmTableSp($module,$toppage->$module);
$urowcont = array();

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$count  = 0;

foreach ( $usrlist as $ussn => $data ) {

  $name   = $data["name"];
  $org    = $data["org"];
  $nhosts = 0; $nhosts = $data["hosts"];
  $jcount = 0; $jcount = $data["jobs"];

  if ( $jcount < $jobnum ) continue; /* In case list only those with jobs */

  $count++;
  $encuname = rawurlencode($ussn);
  $usrwin   = popup("userlist.php?owner=$encuname",700,500,5);
  $urowcont[] = $count;
  $urowcont[] = "<a href=\"$usrwin\">$name</a>";
  $urowcont[] = $org;
  $urowcont[] = $jcount;
  $urowcont[] = $nhosts;
  $utable->addrow($urowcont);
  $urowcont = array();
}
$utable->close();

$toppage->close();

return 0;

// Done



?>
