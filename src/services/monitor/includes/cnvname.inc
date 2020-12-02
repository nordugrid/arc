<?php

// Author: oxana.smirnova@hep.lu.se

// Some name conversion

/**
 * @return string
 * @param uname string
 * @param flag integer
 * @desc Takes user DN and attempts to extract a human name
 */
function cnvname ( $uname, $flag="0" ) 
{

  $uname   = trim($uname);                       /* Just for the case, trimming whitespaces */
  $needle  = "CN=";
  $cn      = substr(stristr($uname, $needle),3); /* gives "John M. Smith 123/blah" */
  $tailpos = strpos($cn, "/");
  if ( $tailpos ) $cn = substr($cn,0,$tailpos);  /* gives "John M. Smith 123" */
  eval("\$cn = \"$cn\";");
  $names = explode(" ",$cn);                     /* gives ("John","M.","Smith","123") */
  $nn    = count($names);                        /* gives 4 */
  $family = $names[$nn-1];                       /* returns "123" */
  if ( $nn > 1 && $family != "Doe") {            /* catch for the tutorials */
    $doestr = substr($family,1,1);               /* returns "1" if it is a number, or a letter if it's a name */
    if ( preg_match("/[0-9]/",$doestr) ) {
      $number = array_pop($names);
      $family = end($names);
    }
    
    //  $family   = substr(strrchr($uname, " "), 1);

    $name = $cn[0].".";                          /* First letter of the name (doesn't work with 8-bit strings) */
    if ( $flag == 2 ) $name = $names[0];
    eval("\$name = \"$name\";"); 

    $family = $name."&nbsp;".$family;
  } else {
    $family = $cn;
  }
  
  if ( !$family ) return $uname                  /* Give up */;
  return $family;
  
}

/**
 * @return string
 * @param uname string
 * @desc Takes user DN and attempts to extract her affiliation
 */
function getorg ( $uname ) 
{

  $uname   = trim($uname);
  $pieces  = explode("/L=", $uname);
  if ( count($pieces) == 1 ) $pieces = explode("/DC=", $uname);  
  if ( count($pieces) == 1 ) $pieces  = explode("/OU=", $uname);
  if ( count($pieces) == 1 ) $pieces  = explode("/O=", $uname);
  $org     = end($pieces);
  $tailpos = strpos($org, "/");
  if ( $tailpos ) $org = substr($org,0,$tailpos);

  return $org;
  
}

?>