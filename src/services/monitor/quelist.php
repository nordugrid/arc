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

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('toreload.inc');
require_once('ldap_nice_dump.inc');

// getting parameters

$host   = ( !empty($_GET["host"]) )  ? $_GET["host"]  : "nordugrid.unibe.ch";
$port   = ( !empty($_GET["port"]) )  ? $_GET["port"]  : 2135;
$qname  = ( !empty($_GET["qname"]) ) ? $_GET["qname"] : "all";
$schema = ( !empty($_GET["schema"]) )  ? $_GET["schema"]  : "";
$debug  = ( !empty($_GET["debug"]) ) ? $_GET["debug"] : 0;
$lang   = @$_GET["lang"];
if ( !$lang )    $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage  = new LmDoc("quelist",$qname." (".$host.")");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;

$clstring = popup("clusdes.php?host=$host&port=$port&schema=$schema",700,620,1,$lang,$debug);

// Header table

$toppage->tabletop("","<b><i>".$toptitle." ".$qname." (<a href=\"$clstring\">".$host."</a>)</i></b>");

$lim = array( "dn", JOB_NAME, JOB_GOWN, JOB_SUBM, JOB_STAT, JOB_COMP, JOB_USET, JOB_USEM, JOB_ERRS, JOB_CPUS, JOB_EQUE );
if ( $schema == "GLUE2") {
  $lim = array( "dn", GJOB_NAME, GJOB_GOWN, GJOB_SUBM, GJOB_STAT, GJOB_COMP, GJOB_USET, GJOB_USEM, GJOB_ERRS, GJOB_CPUS, GJOB_EQUE );
}
  
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
if ( $schema == "GLUE2") {
  $filstr = "(objectclass=".GOBJ_AJOB.")";
  $dn     = "GLUE2GroupID=services,".DN_GLUE;
}
  
// Establish connection to the requested LDAP server
  
$ldapuri = "ldap://".$host.":".$port;
$ds = ldap_connect($ldapuri);
if ($ds) {
    
  // If contact OK, search for NorduGrid clusters
    
  $basedn = QUE_NAME."=".$qname.",".CLU_NAME."=".$host.",";
  $locdn  = $basedn.$dn;
  if ( $schema == "GLUE2") {
    $basedn = GQUE_NAME."=".$qname.",".GCLU_NAME."=".$host.",";
    $basedn = "GLUE2ShareID=urn:ogf:ComputingShare:".$host.":".$qname.",GLUE2ServiceID=urn:ogf:ComputingService:".$host.":arex,";
    $locdn  = $basedn.$dn;
  }

  $aaa = ldap_nice_dump($strings,$ds,$locdn);
  echo "<br>";    
    
  $ts1 = time();
  $sr  = ldap_search($ds,$dn,$filstr,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["110"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
  // Fall back to conventional LDAP
  //    if (!$sr) $sr = ldap_search($ds,$dn,$filstr,$lim,0,0,$tlim,LDAP_DEREF_NEVER);

  if ($sr) {
    $nmatch = ldap_count_entries($ds,$sr);
    if ($nmatch > 0) {
      $entries = ldap_get_entries($ds,$sr);
      $njobs   = $entries["count"];
        
      define("CMPKEY",JOB_SUBM);
      if ( $schema == "GLUE2") define("CMPKEY",GJOB_SUBM);
      usort($entries,"ldap_entry_comp");
        
      // HTML table initialisation
        
      $ltable = new LmTable($module,$toppage->$module);
        
      // loop on jobs
        
      $nj = 0;
      for ($i=1; $i<$njobs+1; $i++) {
        if ( $schema == "GLUE2") {
	  $equeue  = $entries[$i][GJOB_EQUE][0];
	  if ( $equeue !== $qname ) {
	    if ( $debug == 2 ) dbgmsg($equeue." != ".$qname);
	    continue;
	  }
	  $jobdn   = rawurlencode($entries[$i]["dn"]);
	  $curstat = $entries[$i][GJOB_STAT][0];
	  $stahead = substr($curstat,0,12);
	  $ftime = "";
	  if ($stahead=="FINISHED at:") {
	    $ftime = substr(strrchr($curstat, " "), 1);
          } elseif ($curstat=="FINISHED") {
	    $ftime = $entries[$i][GJOB_COMP][0];
	  }
	  if ( $ftime ) {
	    $ftime   = cnvtime($ftime);
	    $curstat = "FINISHED at: ".$ftime;
	  }
	  $uname    = $entries[$i][GJOB_GOWN][0];
	  $encuname = rawurlencode($uname);
	  $family   = cnvname($uname, 2);
          
	  $jname   = htmlentities($entries[$i][GJOB_NAME][0]);
	  $jobname = ($entries[$i][GJOB_NAME][0]) ? $jname : "<font color=\"red\">N/A</font>";
	  $time    = ($entries[$i][GJOB_USET][0]) ? $entries[$i][GJOB_USET][0] : "";
	  $memory  = ($entries[$i][GJOB_USEM][0]) ? $entries[$i][GJOB_USEM][0] : "";
	  $ncpus   = ($entries[$i][GJOB_CPUS][0]) ? $entries[$i][GJOB_CPUS][0] : "";
	  $error   = ($entries[$i][GJOB_ERRS][0]);
	  if ( $error ) $error = ( preg_match("/user/i",$error) ) ? "X" : "!";
	  $status  = "All";
	  $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jobdn&schema=$schema",750,430,4,$lang,$debug);
	  $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname&schema=$schema",700,500,5,$lang,$debug);
        } else {
          //NG schema parse
          $equeue  = $entries[$i][JOB_EQUE][0];
          if ( $equeue !== $qname ) {
            if ( $debug == 2 ) dbgmsg($equeue." != ".$qname);
            continue;
          }
          $jobdn   = rawurlencode($entries[$i]["dn"]);
          $curstat = $entries[$i][JOB_STAT][0];
          $stahead = substr($curstat,0,12);
          $ftime = "";
          if ($stahead=="FINISHED at:") {
            $ftime = substr(strrchr($curstat, " "), 1);
          } elseif ($curstat=="FINISHED") {
            $ftime = $entries[$i][JOB_COMP][0];
          }
          if ( $ftime ) {
            $ftime   = cnvtime($ftime);
            $curstat = "FINISHED at: ".$ftime;
          }
          $uname    = $entries[$i][JOB_GOWN][0];
          $encuname = rawurlencode($uname);
          $family   = cnvname($uname, 2);

          $jname   = htmlentities($entries[$i][JOB_NAME][0]);
          $jobname = ( !empty($entries[$i][JOB_NAME][0]) ) ? $jname : "<font color=\"red\">N/A</font>";
          $time    = ( !empty($entries[$i][JOB_USET][0]) ) ? $entries[$i][JOB_USET][0] : "";
          $memory  = ( !empty($entries[$i][JOB_USEM][0]) ) ? $entries[$i][JOB_USEM][0] : "";
          $ncpus   = ( !empty($entries[$i][JOB_CPUS][0]) ) ? $entries[$i][JOB_CPUS][0] : "";
          $error   = ( !empty($entries[$i][JOB_ERRS][0]) );
          if ( $error ) $error = ( preg_match("/user/i",$error) ) ? "X" : "!";
          $status  = "All";
          $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jobdn",750,430,4,$lang,$debug);
          $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname",700,500,5,$lang,$debug);
        }
          
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
      return $errno;
    }
  } else {
    $errno = "5";
    echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
    return $errno;
  }
  ldap_free_result($sr);
  ldap_close($ds);
  return 0;
}
else {
  $errno = "6";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

// Done

$toppage->close();

?>
