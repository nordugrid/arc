<?php

// Author: oxana.smirnova@hep.lu.se

/**
 * A  function purging any LDAP
 * search result with identical DNs
 */

/**
* @return array
* @param allentries array
* @param attr string
* @desc Purges identical $attr entries from LDAP search result
*/
function ldap_purge($allentries,$attr="dn",$debug=0)
{
  
  $dnstore = array(); /* array to store unique attributes */
  $entries = array(); /* purged array to be returned */

  $ninp = $allentries["count"];
  
  $storesize = 0;

  for ($i = 0; $i < $ninp; $i++) {
    $curdn = $allentries[$i][$attr][0];
    if ( $attr == "dn" ) $curdn = $allentries[$i][$attr];
    // $trimdn = str_replace(" ","",$curdn); // get rid of blanks in DN
    $trimdn = $curdn;
    if ( !in_array($trimdn,$dnstore) ) {
      array_push($dnstore,$trimdn);
      $storesize++;
      array_push($entries,$allentries[$i]);
    } else {
      //      if ( $debug==2 ) echo "<b>### purged DN:".$curdn."</b><br>\n";
    }
  }

  $entries["count"] = $storesize;

  return $entries;

}

?>
