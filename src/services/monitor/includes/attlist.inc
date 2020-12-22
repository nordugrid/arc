<?php

require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('recursive_giis_info.inc');
require_once('archery.inc');
require('ldap_purge.inc');

/**
 * @return int
 * @param attributes array
 * @param signs array
 * @param filters array
 * @param strings array
 * @desc Retrieves the list of objects satisfying attr1ibute/filter selection criteria
 */
function do_attlist ($thething,$attributes,$signs,$filters,$strings,$giislist,$archery_list)
{
  $errors = &$strings["errors"];
  $isattr = &$strings["isattr"];

  if ( !$thething ) $thething = "cluster";

  // Header table
  
  (new LmDoc)->tabletop("",TOPTIT);
  
  $tlim = 20;
  $tout = 20;

  $engatts = array ();
  $lim     = array ( "dn" );

  // deduce the top object name assuming all requests are for the same object
  $components = explode("-",$attributes[0]);
  $object     = $components[0]."-".$components[1];

  // start building the filter
  $filter     = "(&(objectclass=$object)";

  $n       = count($attributes);
  $natt    = 0;
  $attrtag = array ();

  for ( $i=0; $i<$n; $i++ ){
    $attribute = $attributes[$i];
    if ( !$attribute ) continue;
    $selection = ( $filters[$i] ) ? $filters[$i] : "*";
    $is        = ( $signs[$i] ) ? $signs[$i] : "=";
    $lim[]     = $attribute;
    if ( !in_array($attribute,$attrtag)) $engatts[] = ( $isattr[$attribute] ) ? $isattr[$attribute] : $attribute;
    $attrtag[] = $attribute;

    // create the filter
    switch ( $is ) {
    case "!=":
      $filter .= "(!($attribute=$selection))";
      break;
    case "~":
      $selection = "*".$selection."*";
      $filter .= "($attribute=$selection)";
      break;
    case "!~":
      $selection = "*".$selection."*";
      $filter .= "(!($attribute=$selection))";
      break;
    default:
      $filter .= "($attribute$is$selection)";
    }
    $natt++;
  }
  $filter .= ")";

  if ( ! empty($giislist) )     {
    $gentries = recursive_giis_info($giislist,$thething,$errors,0,1);
    $nc       = count($gentries);
  }
  // TODO: fix for GLUE2 schema
  if ( ! empty($archery_list) ) $gentries = array_merge($gentries, archery_info($archery_list, "NG", $errors, 0));

  if ( !$nc ) {
    $errno = 1;
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

  // Search all clusters

  $srarray = @ldap_search($dsarray,DN_LOCAL,$filter,$lim,0,0,$tlim,LDAP_DEREF_NEVER);

  echo "<!-- LDAP filter used: $filter -->\n";
  
  // HTML table initialisation
  array_unshift($engatts,$errors["425"]);
  $jtable  = new LmTableFree($engatts);
  $rowcont = array();
  $tabcont = array();
  $rc = 0;
         
  for ( $ids = 0; $ids < $nhosts; $ids++ ) {

    $sr  = $srarray[$ids];
    $dst = $dsarray[$ids];
    $pn  = $pnarray[$ids];

    if ($dst && $sr) {
       
      // If search returned, check that there are valid entries
       
      $nmatch = @ldap_count_entries($dst,$sr);
      if ($nmatch > 0) {
        // If there are valid entries, tabulate results
         
        $allentries = ldap_get_entries($dst,$sr);
	$entries    = ldap_purge($allentries);
	if ( $object == OBJ_AJOB ) {
	  define("CMPKEY",JOB_STAT);
	  //usort($entries,"ldap_entry_comp");
	}
        $nclus = $entries["count"];
        
        for ($i=0; $i<$nclus; $i++) {

	  $cluster = "N/A";
	  $queue   = "N/A";
	  $job     = "N/A";

	  $currdn  = $entries[$i]["dn"];
	  $currdn  = preg_replace("/\"/","",$currdn);
	  $dnparts = ldap_explode_dn($currdn,0);
	  foreach ($dnparts as $part) {
	    $pair = explode("=",$part);
	    switch ( $pair[0] ) {
	    case CLU_NAME:
	      $cluster = $pair[1];
	      break;
	    case SEL_NAME:
	      $se      = $pair[1];
	      break;
	    case QUE_NAME:
	      $queue   = $pair[1];
	      break;
	    case JOB_GLID:
	      $job     = $pair[1];
              $encjob  = rawurlencode($currdn);
	      break;
	    }
	  }

	  $sort = "cluster";
	  // 410: cluster; 411: queue; 412: job; 413: user; 414: SE
	  switch ( $object ) {
	  case OBJ_CLUS:
	    $resource  = $errors["410"]." <i>$cluster</i>";
            $winstring = popup("clusdes.php?host=$cluster&port=$pn",700,620,1);
	    break;
	  case OBJ_QUEU:
	    $resource  = $errors["410"]." <i>$cluster</i>, ".$errors["411"]." <i>$queue</i>";
	    $winstring = popup("quelist.php?host=$cluster&port=$pn&qname=$queue",750,430,6);
	    break;
	  case OBJ_USER:
	    $resource  = $errors["410"]." <i>$cluster</i>, ".$errors["411"]." <i>$queue</i>";
	    $winstring = popup("quelist.php?host=$cluster&port=$pn&qname=$queue",750,430,6);
	    break;
	  case OBJ_AJOB:
	    $resource  = $errors["412"]." <i>$job</i>";
	    $winstring = popup("jobstat.php?host=$cluster&port=$pn&status=&jobdn=$encjob",750,430,4);
	    break;
	  case OBJ_STEL:
	    $resource  = $errors["414"]." <i>$se</i>";
	    $winstring = "";
	    break;
	  }
	  
	  $rc++;
          $rowcont[0] = ( $winstring ) ? "$rc&nbsp;<a href=\"$winstring\">$resource</a>" : "$rc&nbsp;$resource";
	  
	  // determine maximum row count per object
	  $vcount = 0;
	  foreach ( $attributes as $attribute ) {
	    if ( !$attribute ) continue;
	    $ccount = $entries[$i][$attribute]["count"];
	    $vcount = ( $ccount > $vcount ) ? $ccount : $vcount;
	  }
	  if ($vcount == 0) $jtable->adderror($resource);
        
	  $attrtag = array();
	  for ( $j = 0; $j < $vcount; $j++ ) {
	    $attval   = "";
	    $attcheck = FALSE;
	    for ( $k = 0; $k < $n ; $k++ ) {
	      $attribute = $attributes[$k];
	      if ( !$attribute || @in_array($attribute,$attrtag[$j]) ) continue;
	      if ( $entries[$i][$attribute][$j] ) {
		$attval   = $entries[$i][$attribute][$j];
		$attcheck = TRUE;
	      } else {
		$attval   = " ";
	      }
	      
	      // Some time-stamp readability adjustment
	      if (substr(strrchr($attribute, "-"), 1) == "sessiondirerasetime" ||
		  substr(strrchr($attribute, "-"), 1) == "submissiontime"      ||
		  substr($attribute,0,9)              == "Mds-valid"
		  ) $attval=cnvtime($attval);
	      $rowcont[]     = htmlentities($attval);
	      $attrtag[$j][] = $attribute;
	    }
	    
	    if ( $attcheck ) {
	      $tabcont[] = $rowcont;
	    } else {
	      $rc--;
	    }
	    //	    if ( $attcheck ) $jtable->addrow($rowcont);
	    $rowcont = array();	    
	    $rowcont[0] = "&nbsp;";
	  }
        }
      }
    }
    @ldap_free_result($sr);
  }
  foreach ( $tabcont as $row ) $jtable->addrow($row,"");
  $jtable->close();
  return 0;
}

?>