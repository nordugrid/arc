<?php

// Author: oxana.smirnova@hep.lu.se
/**
 * Simple list of SEs and some of their parameters - stripped down version of loadmon.php
 */

include_once 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('toreload.inc');

// getting parameters

$debug = ( $_GET["debug"] )  ? $_GET["debug"]  : 0;

// Setting up the page itself

$toppage = new LmDoc("sestat");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$isislist = &$toppage->isislist;
$cert     = &$toppage->cert;

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

// Top ISIS server: get all from the pre-defined list

$nisis = count($isislist);
$type = 'DB';
$isis = new $type($cert,$isislist);

$ts1   = time();
$isis->connect($debug, "storage");

$gentries = $isis->get_infos();
$ts2   = time(); if($debug) dbgmsg("<br><b>".$errors["106"].$nisis." (".($ts2-$ts1).$errors["104"].")</b><br>");

$nc = count($gentries);

if ( !$nc ) {
  // NO SITES FOUND!
  $errno = "1";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

$dsarray = array ();
$hnarray = array ();
$sitetag = array (); /* a tag to skip duplicated entries */

$ncc = 0;
foreach ( $gentries as $vo) {
    $sitec = count($vo)/2;
    $ncc += $sitec;
    for ( $i = 0; $i < $sitec; $i++ ) {
        array_push($hnarray,(string)$vo[(string)$vo[$i]]["EPR"]);
    }
}

for ( $k = 0; $k <  count($hnarray); $k++ ) {
  $clport = $gentries[$k]["port"];

  $clhost = $hnarray[$k];
  $clconn = $isis->cluster_info($clhost,$debug);

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
  $toppage->close();
  return $errno;
}

$ctable = new LmTableSp($module,$toppage->$module);

// Loop on SEs

$senum = 0;
$space = 0;
$capacity = 0;
for ( $ids = 0; $ids < $nhosts; $ids++ ) {

  $ds = $dsarray[$ids];
  $hn = $hnarray[$ids]; /* host name, for debugging */
  if ($ds) {

    $entries   = $ds->AdminDomain->Services->StorageService;
    $nclusters = count($entries);      /* May be several SEs! */
    if ( !$nclusters ) continue;

    for ( $i = 0; $i < $nclusters; $i++) {
      $senum++;
      $curdn       = $entries[$i]["dn"];//TODO:
      $curname     = $entries[$i][SEL_NAME][0];//TODO:
      $curalias    = $hn;
      $curspace    = ( $entries[$i]->StorageServiceCapacity->FreeSize ) ? $entries[$i]->StorageServiceCapacity->FreeSize : 0;
      $curcapacity = ( $entries[$i]->StorageServiceCapacity->TotalSize ) ? $entries[$i]->StorageServiceCapacity->TotalSize : $curspace;
      $cururl      = ( $entries[$i][SEL_BURL][0] ) ? $entries[$i][SEL_BURL][0] : $hn;
      $curtype     = $entries[$i]->Type;
      $clstring    = popup("clusdes.php?host=$curname&port=$curport&isse=1&debug=$debug",700,620,1);

      //$curspace  = intval($curspace/1000);
      $occupancy = 1; // by default, all occupied
      $space   += $curspace;
      //      if ( $curcapacity != $errors["407"] ) {
      if ( $curcapacity != 0 ) {
        //$curcapacity = intval($curcapacity/1000);
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
      if ( strlen($curalias) > 30 ) $curalias = substr($curalias,8,22) . ">";
            $clstring  = popup("clusdes.php?host=$curname&port=2135",700,620,1);

      $rowcont[] = "$senum";
      //$rowcont[] = "<a href=\"$clstring\">&nbsp;<b>$curalias</b></a>";
      $rowcont[] = "<b>$curalias</b>";

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

$toppage->close();
return 0;

// Done

$toppage->close();

?>
