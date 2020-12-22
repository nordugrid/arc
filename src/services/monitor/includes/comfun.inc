<?php

// Author: oxana.smirnova@hep.lu.se

// Collection of comparison functions

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares LDAP entries
 */
function ldap_entry_comp($a, $b)
{
  if ( !empty($a[CMPKEY][0]) && !empty( $b[CMPKEY][0]) ) {
    return strcasecmp ($a[CMPKEY][0],$b[CMPKEY][0]);
  } else {
    return 0;
  }  
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares DN strings
 */
function dncmp($a, $b)
{
  return strcasecmp ($a["dn"],$b["dn"]);
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares queues by maxcputime
 */
function quetcmp ($a, $b) 
{
  if (gettype($a) != "array") return -1;
  if (gettype($b) != "array") return 1;
  if ( !empty($a[CMPKEY][0]) && !empty( $b[CMPKEY][0]) ) {
    $tm1 = ($a[CMPKEY][0])?$a[CMPKEY][0]:999999;
    $tm2 = ($b[CMPKEY][0])?$b[CMPKEY][0]:999999;
    return $tm1-$tm2;
  } else {
    return 0;
  }
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares VO strings, or else host names
 */
function vocmp ($a, $b) 
{
  //  $cmpvo   = strcasecmp ($a["zvoname"],$b["zvoname"]);
  $cmpvo   = strcasecmp ($a["zvolocale"],$b["zvolocale"]);
  $cmpname = strcasecmp ($a["host"],$b["host"]);
  if ( !$cmpvo ) return $cmpname;
  return $cmpvo;
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Sorts a list of human names specified as N.Ame (used in allusers.php)
 */
function hncmp ($a, $b) 
{
  $name1  = $a["name"];
  $name2  = $b["name"];
  $lname1 = substr(strrchr($name1,"&nbsp;"),1);
  $lname2 = substr(strrchr($name2,"&nbsp;"),1);

  return strcasecmp ($lname1,$lname2);
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares countries, or else host aliases
 */
function comp_by_country ($a, $b) 
{

  /*
   * Must deal with a beast like this:
   * <a href="/monitor/loadmon1.php?debug=0&display=vo=Norway"><img src="icons/Norway.png" title="XXX" height="10" width="16" border="0">&nbsp;<i><b>XXX</b></i></a>&nbsp&nbsp;</font></nobr>
   */

  $geo1 = $a[0]; $geo2 =  $b[0];
  $geo1 = substr(stristr($geo1,"<b>"),3); $geo2 = substr(stristr($geo2,"<b>"),3);
  $geo1 = substr($geo1,0,strpos($geo1,"<")); $geo2 = substr($geo2,0,strpos($geo2,"<"));

  $ali1 = $a[1]; $ali2 =  $b[1];
  $ali1 = substr(stristr($ali1,"<b>"),3); $ali2 = substr(stristr($ali2,"<b>"),3);
  $ali1 = substr($ali1,0,strpos($ali1,"<")); $ali2 = substr($ali2,0,strpos($ali2,"<"));

  $cmpgeo = strcasecmp ($geo1,$geo2);
  $cmpali = strcasecmp ($ali1,$ali2);
  if ( !$cmpgeo ) return $cmpali;
  return $cmpgeo;
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares by CPU
 */
function comp_by_cpu ($a, $b) 
{
  $cpu1 = $a[2]; $cpu2 =  $b[2];
  $cmpcpu = $cpu2 - $cpu1;
  return $cmpcpu;
}

/**
 * @return int
 * @param a array
 * @param b array
 * @desc Compares by grid running jobs
 */
function comp_by_grun ($a, $b) 
{

  $sum1 = $a[3]; $sum2 =  $b[3];
  //  echo $sum1." vs ".$sum2."<br>";
  $sum1 = substr(stristr($sum1,"alt=\""),5); $sum2 = substr(stristr($sum2,"alt=\""),5);
  $sum1 = substr($sum1,0,strpos($sum1,"+")); $sum2 = substr($sum2,0,strpos($sum2,"+"));

  $cmpsum = $sum2 - $sum1;
  return $cmpsum;
}

?>