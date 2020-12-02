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

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_nice_dump.inc');
require_once('recursive_giis_info.inc');
require('ldap_purge.inc');

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

$owner = @$_GET["owner"];
$bdn   = DN_GLOBL;
$debug = ( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

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
$giislist = &$toppage->giislist;

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

// ldapsearch filter string for jobs

$filter  = "(&(objectclass=".OBJ_USER.")(".USR_USSN."=$uname))";
$ufilter = "(&(objectclass=".OBJ_AJOB.")(".JOB_GOWN."=$uname))";

$gentries = recursive_giis_info($giislist,"cluster",$errors,$debug);
$nc       = count($gentries);
if ( !$nc ) {
  $errno = "1";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

$dsarray = array ();
$hnarray = array ();
$pnarray = array ();
$sitetag = array (); /* a tag to skip duplicated entries */

for ( $k = 0; $k < $nc; $k++ ) {
  $clhost = $gentries[$k]["host"];
  $clport = $gentries[$k]["port"];
  $ldapuri = "ldap://".$clhost.":".$clport;

  $clconn = ldap_connect($ldapuri);
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
  return $errno;
}

// Search all clusters for (a) allowed queues and (b) for user jobs

$uiarray = @ldap_search($dsarray,DN_LOCAL,$filter,$lim,0,0,$tlim,LDAP_DEREF_NEVER);

// Loop on results: first go queues

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

  $ui    = $uiarray[$ids];
  $hn    = $hnarray[$ids];
  $pn    = $pnarray[$ids];
  $dst   = $dsarray[$ids];
  $curl  = popup("clusdes.php?host=$hn&port=$pn",700,620,1,$lang,$debug);

  if ($dst && $ui) {

    $nqueues = @ldap_count_entries($dst,$ui);
    if ($nqueues > 0) {

      $nauclu++;
      array_push($goodds,$dst);
      array_push($goodhn,$hn);
      array_push($goodpn,$pn);
				
      // If there are valid entries, tabulate results
				
      $allres  = ldap_get_entries($dst,$ui);
      $results = ldap_purge($allres);
      $nqueues = $allres["count"];
				
      //   define("CMPKEY",USR_CPUS);
      //   usort($allres,"ldap_entry_comp");
				
      // loop on queues
				
      for ($j=0; $j<$nqueues; $j++) {
	$parts = ldap_explode_dn($allres[$j]["dn"],0);
	foreach ($parts as $part) {
	  $pair = explode("=",$part);
	  switch ( $pair[0] ) {
	  case CLU_NAME:
	    $ucluster = $pair[1];
	    break;
	  case QUE_NAME:
	    $uqueue   = $pair[1];
	    break;
	  }
	}

	if ( $debug == 2 ) dbgmsg("$hn -- $ucluster<br>");

	$qurl  = popup("quelist.php?host=$ucluster&port=$pn&qname=$uqueue",750,430,6,$lang,$debug);
	$curl  = popup("clusdes.php?host=$ucluster&port=$pn",700,620,1,$lang,$debug);
	$fcpu  = $allres[$j][USR_CPUS][0];
	$fproc = freeproc($fcpu);
	$fdisk = $allres[$j][USR_DISK][0];
	$exque = $allres[$j][USR_QUEU][0];
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
  @ldap_free_result($ui);
}

$utable->adderror("<font color=\"#ffffff\"><i><b>".$errors["421"].$nauclu.$errors["422"]."</b></i></font>", "#0099FF");


$utable->close();
echo "<br>\n";

$srarray = @ldap_search($goodds,DN_LOCAL,$ufilter,$ulim,0,0,$tlim,LDAP_DEREF_NEVER);

// HTML table initialisation
$jtable  = new LmTable($module,$toppage->$module);
$rowcont = array();
$jcount  = 0;

$nghosts = count($goodds);

for ( $ids = 0; $ids < $nghosts; $ids++ ) {

  $sr  = $srarray[$ids];
  $dst = $goodds[$ids];
  $gpn = $goodpn[$ids];
  $ghn = $goodhn[$ids];

  if ($dst && $sr) {
			
    // If search returned, check that there are valid entries
			
    $nmatch = @ldap_count_entries($dst,$sr);
    if ($nmatch > 0) {
      // If there are valid entries, tabulate results
				
      $allentries = ldap_get_entries($dst,$sr);
      $entries    = ldap_purge($allentries);
      $njobs      = $entries["count"];
				
      define("CMPKEY",JOB_SUBM);
      usort($entries,"ldap_entry_comp");
				
      // loop on jobs
				
      for ($i=1; $i<$njobs+1; $i++) {
	$jobdn   = rawurlencode($entries[$i]["dn"]);
	$curstat = $entries[$i][JOB_STAT][0];
	$stahead = substr($curstat,0,12);
	if ($stahead=="FINISHED at:") {
	  $ftime   = substr(strrchr($curstat, " "), 1);
	  $ftime   = cnvtime($ftime);
	  $curstat = "FINISHED at: ".$ftime;
	}
	$jname    = htmlentities($entries[$i][JOB_NAME][0]);
	$jobname  = ($entries[$i][JOB_NAME][0]) ? $jname : "<font color=\"red\">N/A</font>";
	$queue    = ($entries[$i][JOB_EQUE][0]) ? $entries[$i][JOB_EQUE][0] : "N/A";
	$cluster  = ($entries[$i][JOB_ECLU][0]) ? $entries[$i][JOB_ECLU][0] : "N/A";
	$time     = ($entries[$i][JOB_USET][0]) ? $entries[$i][JOB_USET][0] : "N/A";
	$ncpus    = ($entries[$i][JOB_CPUS][0]) ? $entries[$i][JOB_CPUS][0] : "";
	$error    = ($entries[$i][JOB_ERRS][0]);
	if ( $error ) $error = ( preg_match("/user/i",$error) ) ? "X" : "!";
	if ( $debug == 2 ) dbgmsg("$ghn --- $cluster<br>");
	$newwin   = popup("jobstat.php?host=$cluster&port=$gpn&status=$status&jobdn=$jobdn",750,430,4,$lang,$debug);
	$quewin   = popup("quelist.php?host=$cluster&port=$gpn&qname=$queue",750,430,6,$lang,$debug);
	$clstring = popup("clusdes.php?host=$cluster&port=$gpn",700,620,1,$lang,$debug);
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
  @ldap_free_result($sr);
}
if ( !$jcount ) $jtable->adderror("<b>".$errors["13"].$family."</b>");
$jtable->close();

return 0;

// Done

$toppage->close();

?>
