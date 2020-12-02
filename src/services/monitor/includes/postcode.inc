<?php

// Author: oxana.smirnova@hep.lu.se (2003)

/**
 * @return string
 * @param zip string
 * @param name string
 * @desc Returns VO (country) name using ZIP code of MDS or the host domain name
 */
function cnvvo($zip,$name)
{

  $oldpath = get_include_path();
  $cwd     = getcwd();
  set_include_path($oldpath.":".$cwd."/lang");
  include("en.inc") ; // Always use "en" locale, for flag icons
  $tlconvert = $message["tlconvert"]; 

  $zparts = explode("-",$zip);
  $tlcode = trim($zparts[0]);
  if ( !$tlcode || strlen($tlcode) !== 2 ) {
    $dname = substr($name,-2);
    $tlcode = strtoupper($dname);
  }
  $match  = ( $tlconvert[$tlcode] ) ? $tlconvert[$tlcode] : "World";
    
  return $match;
}

/**
 * @return array
 * @param cllist array 
 * @desc Contacts clusters in the input lists and adds geographical location info ("zvoname")
 */
function add_country($cllist)
{

  if ( !defined("OBJ_CLUS") ) define("OBJ_CLUS","nordugrid-cluster");
  if ( !defined("CLU_ZIPC") ) define("CLU_ZIPC","nordugrid-cluster-location");
  if ( !defined("DN_LOCAL") ) define("DN_LOCAL","mds-vo-name=local,o=grid");

  $tlim = 2;
  $tout = 10;

  $conarr = array();
  $idxarr = array();

  $nclus  = count($cllist);

  for ( $l = 0; $l < $nclus; $l++ ) {
    $host   = $cllist[$l]["host"];
    $port   = $cllist[$l]["port"];
    $ldapuri = "ldap://".$host.":".$port;
    $clconn = ldap_connect($ldapuri);
    array_push($conarr,$clconn);      
    array_push($idxarr,$l);      
  }
  $listres = @ldap_list($conarr,DN_LOCAL,"(objectclass=".OBJ_CLUS.")",array(CLU_ZIPC),0,0,$tlim,LDAP_DEREF_NEVER);
  // Fall back to a conventional LDAP
  //  if ( !$listres[0] ) $listres = ldap_list($conarr,DN_LOCAL,"(objectclass=".OBJ_CLUS.")",array(CLU_ZIPC),0,0,$tlim,LDAP_DEREF_NEVER);

  $nres = count($listres);

  for ( $n = 0; $n < $nres; $n++ ) {
    $res = $listres[$n];
    $con = $conarr[$n];
    $idx = $idxarr[$n]; /* normally, $idx should be equal to $n */

    // attempt to use domain name for country code
    $curnam = $cllist[$idx]["host"];
    $curcod = "";
    $cllist[$idx]["zvoname"] = cnvvo($curcod,$curnam);

    // overwrite the previous decision if country code is set in the postal code
    $record   = @ldap_get_entries($con,$res);
    //    if ( !$record ) $cllist[$idx]["zvoname"] = "<font color=red>$tout sec t/o</a></font>";
    if ( !$record ) continue;
    $nrecords = $record["count"]; /* should be 1 */
    
    for ($m = 0; $m < $nrecords; $m++) {
      $curcod = $record[$m][CLU_ZIPC][0];
      if ( $curcod ) $cllist[$idx]["zvoname"] = cnvvo($curcod,$curnam);
    }
  }
  
  return($cllist);
}

/**
 * @return string
 * @param curnam string
 * @desc Guesses geographical location of a cluster
 */
function guess_country($curnam, $zip)
{
  // Dumb domain name guess by 2 last letters
  $zvoname = cnvvo("",$curnam);
  
  // overwrite the previous decision if country code is set in the postal code
  if ( $zip ) $zvoname = cnvvo($zip,$curnam);
  
  return $zvoname;
}

?>