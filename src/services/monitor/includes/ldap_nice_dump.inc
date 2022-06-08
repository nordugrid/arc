<?php
 
// Author: oxana.smirnova@hep.lu.se

/**
 * Quite a universal function, dumping any LDAP
 * search result in a nice fashion
 */

require_once('mylo.inc');

/**
 * @return string
 * @param ds string
 * @param dn string
 * @param exclude array
 * @desc Dumps all the LDAP attributes (except those in exclude) returned by a search in an HTML table
 */
function ldap_nice_dump() {

  $tlim = 10;
  $tout = 11;
  $exclude = array();

  $strings = func_get_arg(0);
  $ds      = func_get_arg(1);
  $dn      = func_get_arg(2);
  if ( func_num_args() == 4 ) $exclude = func_get_arg(3);

  // Descriptive array of known MDS attributes (optional)
  $mdsattr = &$strings["mdsattr"];
  $isattr  = &$strings["isattr"];
  $errors  = &$strings["errors"];

  // Strings for a list box
  $itself = $_SERVER['PHP_SELF'];
  $fhead = "<form action=\"$itself\" method=\"post\"><textarea readonly name=\"l\" rows=\"4\" cols=\"60\" style=\"font-size:small\">";
  $ftail = "</textarea></form>";

  // Plain LDAP dump for the DN
  $filstr = "(objectclass=*)";
  if ( strpos($dn, DN_GLUE) ) {
      $filstr = "(|(objectClass=".GOBJ_CLUS.")(objectClass=".GOBJ_MAN.")(objectClass=".GOBJ_LOC.")(objectClass=".GOBJ_QUEU.")(objectClass=".GOBJ_CON.")
                   (objectClass=".GOBJ_ADMD.")(objectClass=".GOBJ_EENV.")(objectClass=".GOBJ_BENC.")(objectClass=".GOBJ_AENV."))";
      if ( strpos(strtolower(" ".$dn), GJOB_GLID) ) {
          $filstr = "(|(objectClass=".GOBJ_AJOB."))";
      }
  }
  $sr     = ldap_search($ds,$dn,$filstr,array("*"),0,0,$tlim,LDAP_DEREF_NEVER);

  if ($sr) {
    
    // If search returned, check that there are valid entries
    
    $nmatch = ldap_count_entries($ds,$sr);
    if ($nmatch > 0) {
      
      // If there are valid entries, tabulate results
      
      $first   = ldap_first_entry($ds,$sr);
      if ( !strpos($dn, DN_GLUE) ) $nmatch = 1;

      for ( $j=0; $j<$nmatch; $j++){
        $entries = ldap_get_attributes($ds,$first);
        $nfields = $entries["count"];
      
        //Attempt to order the contents for GLUE2. Maybe needs a custom sort, but didn't seem to work so far.
         if ( strpos($dn, DN_GLUE) ) array_multisort($entries,SORT_DESC,SORT_NATURAL);
      
        // get the Distinguished Name
      
        $thisdn = ldap_get_dn($ds,$first);
        // HTML table initialisation
      
        $dtable = new LmTableSp("ldapdump",$strings["ldapdump"]);
      
        // add the DN entry
      
        $drowcont = array("<b>".$errors["420"]."</b>",$thisdn);
        $dtable->addrow($drowcont, "#cccccc");
        $drowcont = array();
      
        // loop on the rest of attributes
      
        for ($i=0; $i<$nfields; $i++) {
          $curatt = $entries[$i];
          if ( $exclude && in_array($curatt,$exclude) ) continue;
          $engatt = ($isattr[$curatt]) ? $isattr[$curatt] : $curatt;
          $nval   = $entries[$curatt]["count"];
        
          $encatt = rawurlencode($curatt);
          $attwin = popup("attlist.php?attribute=$encatt",650,300,7);

          $attstring   = @( $mdsattr[$curatt] ) ? "<b>$engatt</b>" : "<a href=\"$attwin\"><b>$engatt</b></a>";
          $drowcont[0] = $attstring;
          $drowcont[1] = "&nbsp;";
        
          if ($nval==0) $dtable->addrow($drowcont);
        
          $drowcont[1] = "";
          if ( $nval > 4 ) $drowcont[1] = $fhead;
          for ($k=0; $k<$nval; $k++) {
            $curval = $entries[$curatt][$k];

            // Strip HTML tags some smart folks are adding
            $curval = strip_tags($curval);
	  
            // Some time-stamp readability adjustment
            if ( strlen($curval) == 15 && $curval[14] == "Z" ) $curval=cnvtime($curval);
	  
            $encval = htmlspecialchars($curval,ENT_QUOTES,"UTF-8");

            // E-mail masquerading for short lists (dunno what to do with long lists)
            if (strpos($curval,"@",1) && $nval<5) {
              $m = mylo ($curval);
              if ( !empty($m[0]) ) $encval = "<script type='text/javascript'>mylo('".$m[0]."','".$m[1]."','".$m[2]."','".$m[3]."')</script><noscript>JS disabled</noscript>";
            }

            if ( $nval > 4 ) {
              $drowcont[1] .= "$encval";
              if ( $k < $nval-1 ) $drowcont[1] .= "\n";
            } else {
              $drowcont[1] .= $encval;
              if ( $k < $nval-1 ) $drowcont[1] .= "<br>&nbsp;";
            }
          }
          if ( $nval > 4 ) $drowcont[1] .= $ftail;
          $dtable->addrow($drowcont);
        }
        $dtable->close();
        echo "<br>";
        $first   = ldap_next_entry($ds,$first);
      }
      ldap_free_result($sr);
      return $thisdn;
    } else {
      $errno = 9;
      echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
      return $errno;
    }
  } else {
    $errno = 5;
    echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
    return $errno;
  }
}
?>
