<?php
 
// Author: oxana.smirnova@hep.lu.se
  /**
   * List all the users of a given VO
   * Uses LDAP functions of PHP
   * 
   * Author: O.Smirnova (June 2002)
   * inspired by the LDAPExplorer by T.Miao
   * 
   * input:
   * host (grid-vo.nordugrid.org)
   * port (389)
   * vo
   * group
   * 
   * output:
   * an HTML table, containing list of users
   */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('toreload.inc');

// getting parameters

$host   = @( $_GET["host"] )   ? $_GET["host"]   : "grid-vo.nordugrid.org";
$port   = @( $_GET["port"] )   ? $_GET["port"]   : 389;
$qname  = @$_GET["vo"];
$group  = @$_GET["group"];
$debug  = @( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage = new LmDoc("vousers");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;

// Header table

$toppage->tabletop("","<b><i>".$toptitle."</i></b>");

// ldap search filter string for jobs

$ufilter = "(objectclass=".OBJ_PERS.")";
$ulim    = array ( "dn", VO_USSN, VO_USCN, VO_DESC, VO_INST, VO_MAIL );

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 10;
$tout = 15;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["114"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// Establish connection to the requested VO server

if( $debug ) dbgmsg($errors["117"].$host.":".$port);

$ldapuri = "ldap://".$host.":".$port;
$ds = ldap_connect($ldapuri);

if ($ds) {
  
  // If contact OK, search for people
  
  $ts1 = time();
  $sr  = @ldap_search($ds,$vo,$ufilter,$ulim,0,0,$tlim,LDAP_DEREF_NEVER,$tout);
  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["125"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
  if ($sr) {
    
    // If search returned, check that there are valid entries
    
    $nmatch = @ldap_count_entries($ds,$sr);
    if ($nmatch > 0) {
      
      // If there are valid entries, tabulate results
      
      $entries = @ldap_get_entries($ds,$sr);
      $nusers  = $entries["count"];
      
      define("CMPKEY",VO_USSN);
      usort($entries,"ldap_entry_comp");
      
      // HTML table initialization
      
      $utable = new LmTable($module,$toppage->$module);
      
      // loop on users
      
      $uscnt = 0;
      for ($i=1; $i<$nusers+1; $i++) {
	$dn = $entries[$i]["dn"];
	if ( $group ) {
	  $newfilter = "(member=$dn)";
	  $newdn     = $group.",".$vo;
	  $newlim    = array("dn");
	  $gcheck    = @ldap_search($ds,$newdn,$newfilter,$newlim,0,0,$tlim,LDAP_DEREF_NEVER,$tout);
	  if ( !ldap_count_entries($ds,$gcheck) ) continue;
	}
	$usname   = $entries[$i][VO_USCN][0];
	//          $usname   = utf2cyr($usname,"n");
	//	  $ussn     = strstr($entries[$i][VO_DESC][0],"/");
	$ussn     = substr(strstr($entries[$i][VO_DESC][0],"subject="),8);
	$ussn     = trim($ussn);
	$encuname = rawurlencode($ussn);
	$org      = $entries[$i][VO_INST][0];
	//	  $org      = utf8_decode($org);
	$mail     = $entries[$i][VO_MAIL][0];
	$mailstr  = "mailto:".$mail;
	$usrwin   = popup("userlist.php?owner=$encuname",700,500,5,$lang,$debug);
	
	// filling the table
	
	$uscnt++;
	$urowcont[] = $uscnt;
	$urowcont[] = "<a href=\"$usrwin\">$usname</a>";
	$urowcont[] = "$org";
	$urowcont[] = "<a href=\"$mailstr\">$mail</a>";
	$utable->addrow($urowcont);
	$urowcont = array ();
      }
      $utable->close();
    }
    else {
      $errno = 10;
      echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
      return $errno;
    }
  } else {
    $errno = 5;
    echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
    return $errno;
  }
  ldap_free_result($sr);
  ldap_close($ds);
  return 0;
} else {
  $errno = 6;
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

// Done

$toppage->close();

?>
