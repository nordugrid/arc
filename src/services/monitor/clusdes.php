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

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang"); 

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_nice_dump.inc');

// getting parameters

$host   = @( $_GET["host"] )   ? $_GET["host"]   : "pikolit.ijs.si";
$port   = @( $_GET["port"] )   ? $_GET["port"]   : 2135;
$isse   = @( $_GET["isse"] )   ? $_GET["isse"]   : 0;
$schema = @( $_GET["schema"] ) ? $_GET["schema"] : "NG";
$debug  = @( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage  = new LmDoc("clusdes",$host);
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;

// Header table

$toppage->tabletop("","<b>".$toptitle." <i>$host</i></b>");
  
// Array defining the attributes to be returned
  
$qlim = array( QUE_NAME, QUE_QUED, QUE_GQUE, QUE_PQUE, QUE_LQUE, QUE_RUNG, QUE_GRUN, 
	       QUE_ASCP, QUE_MAXT, QUE_MINT, QUE_STAT );
   
// ldapsearch filter strings for cluster and queues
   
$qfilter = "(objectclass=".OBJ_QUEU.")";
$dn      = DN_LOCAL;
if ($schema == "GLUE2") {
    $qlim = array( GQUE_NAME, GQUE_MAPQ, GQUE_STAT, GQUE_RUNG, GQUE_MAXR, GQUE_LQUE, GQUE_LRUN,
                   GQUE_PQUE, GQUE_QUED, GQUE_MAXQ, GQUE_MINT, GQUE_MAXT );

    // ldapsearch filter strings for cluster and queues

    $qfilter = "(objectclass=".GOBJ_QUEU.")";
    $dn      = DN_GLUE;
}

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
$ldapuri = "ldap://".$chost.":".$port;
$ds = ldap_connect($ldapuri);

if ($ds) {
     
  // If contact OK, search for clusters
     
  $ts1 = time();
  if ( $isse ) {
    $exclude = array(SEL_USER);
    if ( $dn == DN_LOCAL ) $thisdn = ldap_nice_dump($strings,$ds,SEL_NAME."=".$host.",".$dn,$exclude);
 /**
  *  Storage not supported in GLUE2
  * if ( $dn == DN_GLUE ) {
  *         $querydn = SEL_NAME."=".$host.":arex,GLUE2GroupID=services,".DN_GLUE;//TODO: change SEL_NAME
  *         $thisdn = ldap_nice_dump($strings,$ds,$querydn,$exclude);
  * }
  */
  // if it is a cluster
  } else {
    if ( $dn == DN_LOCAL ) $thisdn = ldap_nice_dump($strings,$ds,CLU_NAME."=".$host.",".$dn);
    if ( $dn == DN_GLUE  ) {
        $querydn = "GLUE2ServiceID=urn:ogf:ComputingService:".$host.":arex,GLUE2GroupID=services,".DN_GLUE;
        $thisdn = ldap_nice_dump($strings,$ds,$querydn);
    }
  }
  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["109"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
  if ( strlen($thisdn) < 4 && $debug ) dbgmsg("<div align=\"left\"><i>".$errors["129"].$thisdn."</i></div><br>");
  echo "<br>";
    
  // Loop on queues/shares (if everything works)

  if ($thisdn != 1 && !$isse) {
    $ts1 = time();
    $qsr = @ldap_search($ds,$dn,$qfilter,$qlim,0,0,$tlim,LDAP_DEREF_NEVER);
    $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["110"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
    // Fall back to conventional LDAP
    //      if (!$qsr) $qsr = @ldap_search($ds,$dn,$qfilter,$qlim,0,0,$tlim,LDAP_DEREF_NEVER);
  }
  if ($qsr) {
       
    // If search returned, check that there are valid entries
       
    $nqmatch = @ldap_count_entries($ds,$qsr);
    if ($nqmatch > 0) {
         
      // If there are valid entries, tabulate results
         
      $qentries = @ldap_get_entries($ds,$qsr);
      $nqueues  = $qentries["count"];
        
      // HTML table initialisation
         
      $qtable = new LmTableSp($module,$toppage->$module,$schema);
        
      // loop on the rest of attributes

      define("CMPKEY",QUE_MAXT);
      usort($qentries,"quetcmp");
      for ($k=1; $k<$nqueues+1; $k++) {
        if ( $dn == DN_LOCAL ) {
	    $qname   =  $qentries[$k][QUE_NAME][0];
	    $qstatus =  $qentries[$k][QUE_STAT][0];
	    //	$queued  =  @$qentries[$k][QUE_QUED][0];
       	$queued  = @($qentries[$k][QUE_QUED][0]) ? ($entries[$k][QUE_QUED][0]) : 0; /* deprecated since 0.5.38 */
	    $locque  = @($qentries[$k][QUE_LQUE][0]) ? ($qentries[$k][QUE_LQUE][0]) : 0; /* new since 0.5.38 */
	    $run     = @($qentries[$k][QUE_RUNG][0]) ? ($qentries[$k][QUE_RUNG][0]) : 0;
	    $cpumin  = @($qentries[$k][QUE_MINT][0]) ? $qentries[$k][QUE_MINT][0] : "0";
	    $cpumax  = @($qentries[$k][QUE_MAXT][0]) ? $qentries[$k][QUE_MAXT][0] : "&gt;";
	    $cpu     = @($qentries[$k][QUE_ASCP][0]) ? $qentries[$k][QUE_ASCP][0] : "N/A";
	    $gridque = @($qentries[$k][QUE_GQUE][0]) ? $qentries[$k][QUE_GQUE][0] : "0";
	    $gmque   = @($qentries[$k][QUE_PQUE][0]) ? ($qentries[$k][QUE_PQUE][0]) : 0; /* new since 0.5.38 */
	    $gridrun = @($qentries[$k][QUE_GRUN][0]) ? $qentries[$k][QUE_GRUN][0] : "0";
	    $quewin  = popup("quelist.php?host=$host&port=$port&qname=$qname&schema=$schema",750,430,6,$lang,$debug);
        }
        if ( $dn == DN_GLUE ) {
            $qname   =  $qentries[$k][GQUE_NAME][0];
            $mapque  =  $qentries[$k][GQUE_MAPQ][0];
            $qstatus =  $qentries[$k][GQUE_STAT][0];
            // Queued
            $queued  = @($qentries[$k][GQUE_QUED][0]) ? ($qentries[$k][GQUE_QUED][0]) : 0;
            $locque  = @($qentries[$k][GQUE_LQUE][0]) ? ($qentries[$k][GQUE_LQUE][0]) : 0;
            $gridque = $queued - $locque;
            if ( $gridque < 0 ) $gridque = 0;
            $gmque   = @($qentries[$k][GQUE_PQUE][0]) ? ($qentries[$k][GQUE_PQUE][0]) : 0;
            // Running
            $run     = @($qentries[$k][GQUE_RUNG][0]) ? ($qentries[$k][GQUE_RUNG][0]) : 0;
            $locrun  = @($qentries[$k][GQUE_LRUN][0]) ? ($qentries[$k][GQUE_LRUN][0]) : 0;
            $gridrun = $run - $locrun;
            if ( $gridrun < 0 ) $gridrun = 0;
            // Limits
            $cpumin  = @($qentries[$k][GQUE_MINT][0]) ? $qentries[$k][GQUE_MINT][0] : "0";
            $cpumax  = @($qentries[$k][GQUE_MAXT][0]) ? $qentries[$k][GQUE_MAXT][0] : "&gt;";
            $cpu     = @($qentries[$k][GQUE_MAXR][0]) ? $qentries[$k][GQUE_MAXR][0] : "N/A";
            
            // This below TODO
            $quewin  = popup("quelist.php?host=$host&port=$port&qname=$qname&schema=$schema",750,430,6,$lang,$debug);
        }
	$gridque = $gridque + $gmque;
	if ( $queued == 0 ) $queued = $locque + $gridque;

	// filling the table

	$qrowcont[] = "<a href=\"$quewin\">$qname</a>";
        if ( !empty($mapque) ) {
          $qrowcont[] = "$mapque";
        }
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
      return $errno;
    }
  }
  elseif ( !$isse ) {
    $errno = 5;
    echo "<br><font color=\"red\"><b>".$errors["5"]."</b></font>\n";
    return $errno;
  }
  @ldap_free_result($qsr);
  @ldap_close($ds);
  return 0;
}
else {
  $errno = 6;
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

// Done

$toppage->close();

?>
