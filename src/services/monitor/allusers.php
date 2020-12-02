<?php
 
// Author: oxana.smirnova@hep.lu.se

/* ALL GRID USERS LISTING */
/**
 * Retrieves the list of authorised users and their jobs
 * Uses LDAP functions of PHP
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

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('recursive_giis_info.inc');
require('ldap_purge.inc');

// getting parameters

$debug  = @( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$jobnum = @( $_GET["limit"] )  ? $_GET["limit"]  : 0;
$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage  = new LmDoc("allusers");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$giislist = &$toppage->giislist;
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
  
// ldapsearch filter string for jobs
	
$filter  = "(objectclass=".OBJ_USER.")"; /* Find all users */
$jfilter = "(objectclass=".OBJ_AJOB.")"; /* Find all jobs */

$gentries = recursive_giis_info($giislist,"cluster",$errors,$debug);
$nc       = count($gentries);
if ( !$nc ) {
  // NO SITES FOUND!
  $errno = "1";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

$dsarray = array ();
$hnarray = array ();
$sitetag = array (); /* a tag to skip duplicated entries */

for ( $k = 0; $k < $nc; $k++ ) {
  $clhost = $gentries[$k]["host"];
  $clport = $gentries[$k]["port"];
  $ldapuri = "ldap://".$clhost.":".$clport;
  
  $clconn = ldap_connect($ldapuri);
  if ( $clconn && !$sitetag[$clhost] ) {
    array_push($dsarray,$clconn);
    array_push($hnarray,$clhost);
    $sitetag[$clhost] = 1; /* filtering tag */
  }
}

$nhosts = count($dsarray);
if ( !$nhosts ) {
  // NO SITES REPLY...
  $errno = "2";
  echo "<BR><font color=\"red\"><B>".$errors[$errno]."</B></font>\n";
  return $errno;
}

// Search all clusters for users

$uiarray = array();
$ts1     = time();
$uiarray = @ldap_search($dsarray,DN_LOCAL,$filter,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
$ts2     = time(); if($debug) dbgmsg("<br><b>".$errors["125"]." (".($ts2-$ts1).$errors["104"].")</b><br>");

// Search all clusters for jobs

$jiarray = array();
$ts1     = time();
$jiarray = @ldap_search($dsarray,DN_LOCAL,$jfilter,$jlim,0,0,$tlim,LDAP_DEREF_NEVER);
$ts2     = time(); if($debug) dbgmsg("<br><b>".$errors["126"]." (".($ts2-$ts1).$errors["104"].")</b><br>");

// Loop on clusters; building user list

$usrlist = array ();

for ( $ids = 0; $ids < $nhosts; $ids++ ) {
    
  $ui  = array (); $ui  = $uiarray[$ids];
  $ji  = array (); $ji  = $jiarray[$ids];
  $dst = array (); $dst = $dsarray[$ids];

  if ($dst && $ui) {

    $nusers = @ldap_count_entries($dst,$ui);
    $njobs  = @ldap_count_entries($dst,$ji);
    if ($nusers > 0 || $njobs > 0) {
				
      // If there are valid entries, tabulate results
				
      $allres  = array(); $allres  = @ldap_get_entries($dst,$ui);
      $results = ldap_purge($allres,USR_USSN,$debug);
      $alljobs = array(); $alljobs = @ldap_get_entries($dst,$ji);
      //	$nusers  = $allres["count"];
      $nusers  = $results["count"];
      $njobs   = $alljobs["count"];
				
      // loop on users, filling $usrlist[$ussn]["name"] and counting $usrlist[$ussn]["hosts"]

      for ($j=0; $j<$nusers; $j++) {
	//	  $ussn   = $allres[$j][USR_USSN][0];
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
	$jdn    = $alljobs[$k]["dn"];
	$jown   = $alljobs[$k][JOB_GOWN][0];
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

  //    if ( $count > 9 ) continue;

  $name   = $data["name"];
  $org    = $data["org"];
  $nhosts = 0; $nhosts = $data["hosts"];
  $jcount = 0; $jcount = $data["jobs"];
 
  if ( $jcount < $jobnum ) continue; /* In case list only those with jobs */
    
  $count++;
  $encuname = rawurlencode($ussn);
  $usrwin   = popup("userlist.php?owner=$encuname",700,500,5,$lang,$debug);
  $urowcont[] = $count;    
  $urowcont[] = "<a href=\"$usrwin\">$name</a>";
  $urowcont[] = $org;
  $urowcont[] = $jcount;
  $urowcont[] = $nhosts;
  $utable->addrow($urowcont);
  $urowcont = array();
}
$utable->close();

return 0;

// Done

$toppage->close();


?>
