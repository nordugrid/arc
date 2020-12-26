<?php
 
// Author: oxana.smirnova@hep.lu.se
/**
 * Simple list of SEs and some of their parameters - stripped down version of loadmon.php
 */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_purge.inc');
require_once('recursive_giis_info.inc');

// getting parameters

$debug = ( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage = new LmDoc("sestat");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$giislist = &$toppage->giislist;

// Header table

$toppage->tabletop("<font face=\"Verdana,Geneva,Arial,Helvetica,sans-serif\">".$toptitle."<br><br></font>","");

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 10;
$tout = 15;
if($debug) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</i></div>");

// Arrays defining the attributes to be returned
  
$lim = array( "dn", SEL_NAME, SEL_ANAM, SEL_CURL, SEL_BURL, SEL_TYPE, SEL_FREE, SEL_TOTA );
   
// ldapsearch filter strings for clusters and queues
   
$filstr = "(objectclass=".OBJ_STEL.")";

// Top GIIS server: get all from the pre-defined list

$ngiis = count($giislist);
$ts1   = time(); $gentries = recursive_giis_info($giislist,"nordugrid-SE",$errors,$debug);
$ts2   = time(); if($debug) dbgmsg("<br><b>".$errors["106"].$ngiis." (".($ts2-$ts1).$errors["104"].")</b><br>");

$nc = count($gentries);

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
    if ($debug==2) dbgmsg("$k - <i>$clhost:$clport </i>");
  }
}

$nhosts = count($dsarray);
if ( !$nhosts ) {
  // NO SITES REPLY...
  $errno = "2";
  echo "<BR><font color=\"red\"><B>".$errors[$errno]."</B></font>\n";
  return $errno;
}

// Search all SEs

$ts1 = time();
$srarray = @ldap_search($dsarray,DN_LOCAL,$filstr,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
$ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["124"]." (".($ts2-$ts1).$errors["104"].")</b><br>");

$ctable = new LmTableSp($module,$toppage->$module);
           
// Loop on SEs

$senum = 0;
$space = 0;
$capacity = 0;
for ( $ids = 0; $ids < $nhosts; $ids++ ) {

  $sr = $srarray[$ids];
  $ds = $dsarray[$ids];
  $hn = $hnarray[$ids]; /* host name, for debugging */
  if ($ds && $sr) {

    $entries   = @ldap_get_entries($ds,$sr);
    $nclusters = $entries["count"];      /* May be several SEs! */
    if ( !$nclusters ) continue;

    for ( $i = 0; $i < $nclusters; $i++) {
      $senum++;
      $curdn       = $entries[$i]["dn"];
      $curname     = $entries[$i][SEL_NAME][0];
      $curalias    = $entries[$i][SEL_ANAM][0];
      $curspace    = ( $entries[$i][SEL_FREE][0] ) ? $entries[$i][SEL_FREE][0] : 0;
      //      $curcapacity = ( $entries[$i][SEL_TOTA][0] ) ? $entries[$i][SEL_TOTA][0] : $errors["407"];
      $curcapacity = ( $entries[$i][SEL_TOTA][0] ) ? $entries[$i][SEL_TOTA][0] : $curspace;
      $cururl      = ( $entries[$i][SEL_BURL][0] ) ? $entries[$i][SEL_BURL][0] : $entries[$i][SEL_CURL][0];
      $curtype     = $entries[$i][SEL_TYPE][0];
      $clstring    = popup("clusdes.php?host=$curname&port=$curport&isse=1&debug=$debug",700,620,1,$lang,$debug);
	
      $curspace  = intval($curspace/1000);
      $occupancy = 1; // by default, all occupied
      $space   += $curspace;
      //      if ( $curcapacity != $errors["407"] ) {
      if ( $curcapacity != 0 ) {
	$curcapacity = intval($curcapacity/1000);
	$occupancy   = ($curcapacity - $curspace)/$curcapacity;
	$capacity   += $curcapacity;
      }
      $tstring = $curspace."/".$curcapacity;
      $tlen    = strlen($tstring);
      if ($tlen<11) {
	$nspaces = 11 - $tlen;
	for ( $is = 0; $is < $nspaces; $is++ ) $tstring .= " ";
      }
      $tstring = urlencode($tstring);

      if ($debug==2) dbgmsg("<i>$senum: <b>$curname</b> at $hn</i><br>");
      if ( strlen($curalias) > 15 ) $curalias = substr($curalias,0,15) . ">";
      //      $clstring  = popup("clusdes.php?host=$curname&port=2135",700,620,1,$lang,$debug);
	
      $rowcont[] = "$senum";
      $rowcont[] = "<a href=\"$clstring\">&nbsp;<b>$curalias</b></a>";

      $rowcont[] = "<img src=\"./mon-icons/icon_bar.php?x=1&xg=".$occupancy."&y=13&text=".$tstring."\" vspace=\"2\" hspace=\"3\" border=\"0\" title=\"$tstring\" alt=\"$tstring\" width=\"200\" height=\"13\"></a>";

      //      $rowcont[] = $curcapacity.$errors["408"];
      //      $rowcont[] = $curspace.$errors["408"];
      $rowcont[] = "$curname";
      $rowcont[] = "$cururl";
      $rowcont[] = "$curtype";
      $ctable->addrow($rowcont);
      $rowcont = array ();
    }
  }
  $entries  = array();
  $jentries = array();
  $gentries = array();
}

$occupancy = ($capacity - $space)/$capacity;
$tstring   = $space."/".$capacity;

$ctable->addspacer("#ffcc33");
$rowcont[] = "&nbsp;";
$rowcont[] = "<span style={text-align:right}><i><b>".$errors["405"]."</b></i></span>";
$rowcont[] = "<img src=\"./mon-icons/icon_bar.php?x=1&xg=".$occupancy."&y=13&text=".$tstring."\" vspace=\"2\" hspace=\"3\" border=\"0\" title=\"$tstring\" alt=\"$tstring\" width=\"200\" height=\"13\"></a>";
//$rowcont[] = "<b><i>$capacity".$errors["408"]."</i></b>";
//$rowcont[] = "<b><i>$space".$errors["408"]."</i></b>";
$rowcont[] = "&nbsp;";
$rowcont[] = "&nbsp;";
$rowcont[] = "&nbsp;";
$ctable->addrow($rowcont, "#ffffff");
$ctable->close();

return 0;

// Done

$toppage->close();

?>
