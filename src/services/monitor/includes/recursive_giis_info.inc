<?php

// Author: oxana.smirnova@hep.lu.se

/**
 * @return array
 * @param giislist array
 * @param element string
 * @param debug integer
 * @param loopcnt integer
 * @desc Returns base EGIIS info recursively down to the lowest level
 */
function recursive_giis_info($giislist,$element,$errors,$debug="0",$loopcnt="1")
{
  /*
   * Array $giislist is assumed to be a 3-dim array of the type:
   * array(
   *       array("host" => "grid.nbi.dk",
   *             "port" => "2135",
   *             "base" => "mds-vo-name=NorduGrid,o=grid"),...)
   */

  $loopcnt++;
  
  $tlim = 2;
  $tout = 5;
  if($debug && count($giislist) < 5) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["114"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><BR>");

  $greg      = array();
  $gfilter   = "(objectclass=mds*)";

  $ngiis     = count($giislist);

  $counter   = 0;
  $tag       = array();
  $dsarray   = array();
  $dnarray   = array();
  $hnarray   = array();
  $entries   = array();
  $blacklist = array();

  @include('blacklist.inc'); // uses blacklist if it is in includes or current path

  // Loop on entered sites
  // If a host is blacklisted, skip
  // If a host is a cluster (ARIS), keep and skip
  // If a host is any other local ARIS, skip

  for ( $ig = 0; $ig < $ngiis; $ig++ ) {
    $ghost = $giislist[$ig]["host"];
    if ( @$blacklist[$ghost] ) {
      if ( $debug ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>".$errors["122"]."$ghost</b></font></div>\n");
      continue;
    }
    $gport = $giislist[$ig]["port"];
    $gbase = $giislist[$ig]["base"];
    if ( preg_match("/$element/i",$gbase) ) {
      // Invent a "fake DN" for host tagging and skip duplicated entries
      $fakedn  = "hn=".$ghost.", ".$gbase;
      if ( @$tag[$fakedn] ) continue;
      $tag[$fakedn] = 1;
      array_push($entries,$giislist[$ig]);
      $counter++;
      continue;
    } elseif ( preg_match("/local/i",$gbase) ) {
      if ( $debug ) dbgmsg("<div align=\"left\"><b>".$errors["115"].$ghost." (".$errors["116"].$element.")</b>");
      continue;
    }
    if ( $debug == 2 ) dbgmsg("<div align=\"left\">".$errors["117"]."<b>$ghost</b>...");
    $fp = @fsockopen($ghost, $gport, $errno, $errstr, 2);
    $ldapuri = "ldap://".$ghost.":".$gport;
    $gconn = ldap_connect($ldapuri);
    if ( $fp && $gconn ) {
      fclose($fp);
      if ( $debug == 2 ) dbgmsg($errors["118"]);
      array_push($dsarray,$gconn);
      array_push($dnarray,$gbase);
      array_push($hnarray,$ghost);
    }
    if ( $debug == 2 ) dbgmsg("</div>");
  }

  // Some debugging printout
  if ( $debug == 2 ) {
    dbgmsg("<div align=\"center\"><br><u>".$errors["119"].$element.": ".$counter."</u><br></div>");
    foreach ( $entries as $num=>$val ) dbgmsg($val["host"].":".$val["base"]."<br>");
  }

  // Check if there is underlying structure

    $srarray = @ldap_read($dsarray,$dnarray,$gfilter,$greg,0,0,$tlim,LDAP_DEREF_NEVER);
  // If using the pached LDAP
  //$srarray = @ldap_read($dsarray,$dnarray,$gfilter,$greg,0,0,$tlim,LDAP_DEREF_NEVER,$tout);

  // Debug: check if something eventualy timeouts or something
  if ( $debug ) {
    $nconns = count($dsarray);
    for ( $ii = 0; $ii < $nconns; $ii++ ) {
      $ldconn = $dsarray[$ii];
      $hnconn = $hnarray[$ii];
      if ( ldap_errno($ldconn) != 0x00 ) {
	$ldaperrmess = ldap_error($ldconn);
	dbgmsg("<i><font color=\"red\">".$errors["120"].$hnconn.": ".$ldaperrmess."</font></i><br>");
      }
    }
  }

  $nhosts  = 0;
  if ( !empty($srarray) ) $nhosts  = count($srarray);

  // If EGIISes are found, loop on contacted EGIISes
  if ( $nhosts ) {

    $truecount = 0;

    for( $ids = 0; $ids < $nhosts; $ids++ ) {
      
      // suppose N hosts answered (nhosts), each returned M lower registrants (nrecords)
      // some of lower registrants are the same and have to be purged
      // and everything should be re-arranged in a new common array
      
      $sr   = $srarray[$ids];
      $ds   = $dsarray[$ids];
      $base = $dnarray[$ids];
      if ($sr) $truecount++;
      
      $record   = @ldap_get_entries($ds,$sr);
      $nrecords = $record["count"];
      
      // Per each contacted EGIIS, loop on potential lower-level EGIISes/clusters
      
      for ($i = 0; $i < $nrecords; $i++) {
	
	$curdn   = $record[$i]["dn"];
	$curhost = $record[$i]["mds-service-hn"][0];
	$curstat = $record[$i]["mds-reg-status"][0];
	$curport = $record[$i]["mds-service-port"][0];
	$cursuff = $record[$i]["mds-service-ldap-suffix"][0];
	// Introduce "fake" DN for tagging purpose - helps skipping sites registering twice
	$fakedn  = "hn=".$curhost.", ".$cursuff;
	if ( @$tag[$fakedn] ) continue;

	if ( @$blacklist[$curhost] ) {
	  if ( $debug>0 ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>".$errors["122"]."$curhost</b></font></div>\n");
	  continue;
	}
	if ( $curstat != "VALID" ) {
	  if ( $debug ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>$curstat".$errors["121"]."$fakedn</b></font></div>\n");
	  continue;
	}
	
	// array_push($entries,$record[$i]);
	$entries[$counter]["host"] = $curhost;
	$entries[$counter]["port"] = $curport;
	$entries[$counter]["base"] = $cursuff;
	if ( $debug == 2 ) dbgmsg("<div align=\"left\">".$errors["123"]."$base: <i>$fakedn</i></div>\n");
	$tag[$fakedn] = 1;
	$counter++;
      }
    }
    
    // Array $entries contains all possible stuff which registers to a EGIIS
    // Keep recursing
    
    if ($truecount && $loopcnt < 10 ) $entries = recursive_giis_info($entries,$element,$errors,$debug,$loopcnt);
  } 

  return $entries;

}

?>
