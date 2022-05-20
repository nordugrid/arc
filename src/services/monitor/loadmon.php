<?php
// Author: oxana.smirnova@hep.lu.se

/** THIS IS THE TOP MONITOR WINDOW
 * Retrieves the Grid load information from the NorduGrid domain
 * Uses LDAP functions of PHP
 *
 * Author: O.Smirnova (May 2002)
 *         inspired by the LDAPExplorer by T.Miao
 *         and curses-based monitor by A.Waananen
 *
 * input:
 * debug level (default 0)
 * display range (default all)
 *
 * output:
 * an HTML table, containing:
 * - list of available clusters
 *   per each cluster:
 *   - total running jobs
 *   - relative load of the cluster (ratio "running jobs"/"CPUs")
 *   - queued jobs
 */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_purge.inc');
require_once('recursive_giis_info.inc');
require_once('emirs_info.inc');
require_once('archery.inc');
require_once('postcode.inc');
require_once('cache.inc');

// getting parameters

$order   = @$_GET["order"];
$display = @$_GET["display"];
$debug   = @$_GET["debug"];
$lang    = @$_GET["lang"];
$schema  = @$_GET["schema"];
if ( !$order )   $order   = "country";
if ( !$display ) $display = "all";
if ( !$debug )   $debug   = 0; 
if ( !$lang )    $lang    = "default"; // browser language
if ( !$schema )  $schema  = "NG";
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage  = new LmDoc("loadmon");
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$giislist = &$toppage->giislist;
$emirslist= &$toppage->emirslist;
$cert     = &$toppage->cert;
$yazyk    = &$toppage->language;
$archery_list = &$toppage->archery_list;

// Header table

$toptit = date("Y-m-d T H:i:s");
$toppage->tabletop("<font face=\"Verdana,Geneva,Arial,Helvetica,sans-serif\">".EXTRA_TITLE." ".$toppage->title."<br><br></font>","<i>$toptit</i>");

//********************* Schema changing ******************************
$other_schema = "GLUE2";
if ( $schema == "GLUE2" ) $other_schema = "NG";
$_GET["schema"] = $other_schema;
$get_options = "";
$keys = array_keys($_GET);
foreach ($_GET as $key => $value) {
    if ( $key == $keys[0] ) {
        $get_options = "?";
    } else {
        $get_options = $get_options."&";
    }
    $get_options = $get_options."$key=$value";
}
//TODO: use translate messages
echo "</center>Current data rendered according to <b> $schema </b> schema.<br>";
echo "<b>Schema switching to: <a href=".$_SERVER['PHP_SELF']."$get_options>$other_schema</a></b><p><center>";

//********************** Legend - only needed for this module *********************
echo "<table width=\"100%\" border=\"0\"><tr><td>\n";
echo "<font size=\"-1\" face=\"Verdana,Geneva,Arial,Helvetica,Sans-serif,lucida\">".$errors["401"].":\n";
echo "<img src=\"./mon-icons/icon_led.php?c1=97&c2=144&c3=0\" vspace=\"1\" hspace=\"3\" border=\"0\" title=\"".$errors["305"]."\" alt=\"".$errors["305"]."\" width=\"14\" height=\"6\">".$errors["402"]."&nbsp;<img src=\"./mon-icons/icon_led.php?c1=176&c2=176&c3=176\" vspace=\"1\" hspace=\"3\" border=\"0\" title=\"".$errors["306"]."\" alt=\"".$errors["306"]."\">".$errors["403"]."</font>\n";
echo "</td><td>";
$sewin    = popup("sestat.php",650,200,8,$lang,$debug);
$discwin  = popup("discover.php",700,400,9,$lang,$debug);
$vostring = popup("volist.php",440,330,11,$lang,$debug);
$usstring = popup("allusers.php",650,700,12,$lang,$debug);
$acstring = popup("allusers.php?limit=1",500,600,12,$lang,$debug);
echo "<div align=\"right\"><nobr>\n";
//******** Authorised users
echo "<a href=\"$usstring\"><img src=\"./mon-icons/icon-folks.png\" width=\"24\" height=\"24\" border=\"0\" title=\"".$errors["307"]."\" alt=\"".$errors["307"]."\"></a>&nbsp;\n";
//******** Active users
echo "<a href=\"$acstring\"><img src=\"./mon-icons/icon-run.png\" width=\"24\" height=\"24\" border=\"0\" title=\"".$errors["308"]."\" alt=\"".$errors["308"]."\"></a>&nbsp;\n";
//******** Search
echo "<a href=\"$discwin\"><img src=\"./mon-icons/icon-look.png\" width=\"24\" height=\"24\" border=\"0\" title=\"".$errors["309"]."\" alt=\"".$errors["309"]."\"></a>&nbsp;\n";
//******** Storage
echo "<a href=\"$sewin\"><img src=\"./mon-icons/icon-disk.png\" width=\"24\" height=\"24\" border=\"0\" title=\"".$errors["310"]."\" alt=\"".$errors["310"]."\"></a>&nbsp;\n";
//******** Virtual Organisations
echo "<a href=\"$vostring\"><img src=\"./mon-icons/icon-vo.png\" width=\"24\" height=\"24\" border=\"0\" title=\"".$errors["311"]."\" alt=\"".$errors["311"]."\"></a>\n";
echo "</nobr></div>\n";
echo "</td></tr></table>\n";
//****************************** End of legend ****************************************

// Some debug output

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
  dbgmsg("<div align=\"left\"><b>ARC ".$toppage->getVersion()."</b></div>");
}

$tcont     = array(); // array with rows, to be sorted
$cachefile = CACHE_LOCATION."/loadmon-$schema-".$yazyk;
$tcont     = get_from_cache($cachefile,120);

// If cache exists, skip ldapsearch

if ( !$tcont || $debug || $display != "all" ) { // Do LDAP search

  $tcont = array();

  // Setting time limits for ldapsearch

  $tlim = 10;
  $tout = 11;
  if($debug) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</i></div>");
  
  // ldapsearch filter string for clusters and queues
  $filter="(|(objectClass=".OBJ_CLUS.")(objectClass=".OBJ_QUEU.")(objectclass=".GOBJ_CLUS.")(objectclass=".GOBJ_QUEU.")(objectClass=".GOBJ_MAN.")(objectClass=".GOBJ_LOC.")(objectClass=".GOBJ_CON.")(objectClass=".GOBJ_ADMD."))";

  // Array defining the attributes to be returned
  $lim  = array( "dn",
                GCLU_ANAM, GCLU_ZIPC, GCLU_TCPU, GCLU_UCPU, GCLU_TJOB, GCLU_PQUE, GCLU_SUPP, GCLU_OWNR,
                GQUE_STAT, GQUE_GQUE, GQUE_LQUE, GQUE_PQUE, GQUE_RUNG, GQUE_LRUN,
                CLU_ANAM, CLU_ZIPC, CLU_TCPU, CLU_UCPU, CLU_TJOB, CLU_QJOB, CLU_PQUE,
                QUE_STAT, QUE_GQUE, QUE_QUED, QUE_LQUE, QUE_PQUE, QUE_RUNG, QUE_GRUN );
  
  // Adjusting cluster display filter
  
  $showvo = "";
  if ( substr($display,0,2) == "vo" ) {
    $showvo = substr(strrchr($display,"="),1);
    if ($debug) dbgmsg("<b> ::: ".$errors["105"]."$showvo</b>");
  }
  if ( $display != "all" && !$showvo ) $filter  = "(&".$filstr."(".$display."))";

  //========================= GET CLUSTER LIST ============================
  $gentries = array();
  // EGIIS
  if ( ! empty($giislist) )     {
      $ngiis    = count($giislist);
      $ts1      = time();
      $gentries = recursive_giis_info($giislist,"cluster",$errors,$debug,1);
      $ts2      = time();
      if($debug) dbgmsg("<br><b>".$errors["106"].$ngiis." (".($ts2-$ts1).$errors["104"].")</b><br>");
  }
  // EMIR
  if ( ! empty($emirslist))     $gentries = emirs_info($emirslist,"cluster",$errors,$gentries,$debug,$cert);
  // ARCHERY
  if ( ! empty($archery_list) ) $gentries = array_merge($gentries, archery_info($archery_list, $schema, $errors, $debug));

  //=======================================================================

  $nc = count($gentries);
  
  if ( !$nc ) {
    // NO SITES FOUND!
    $errno = "1";
    echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
    return $errno;
  } else {
      if ( $debug == 2 ) {
          dbgmsg("<div align=\"center\"><br><u>".$errors["119"]."cluster: ".$nc."</u><br></div>");
          foreach ( $gentries as $num=>$val ) dbgmsg($val["host"].":".$val["base"]."<br>");
      }
  }
  
  $dsarray = array ();
  $hnarray = array ();
  $pnarray = array ();
  $dnarray = array ();
  $sitetag = array (); /* a tag to skip duplicated entries */
  
  // Purging cluster entries
  
  for ( $k = 0; $k < $nc; $k++ ) {
    $clhost = $gentries[$k]["host"];
    $clport = $gentries[$k]["port"];
    $basedn = $gentries[$k]["base"];
    $fp = @fsockopen($clhost, $clport, $errno, $errstr, 2);
    $ldapuri = "ldap://".$clhost.":".$clport;
    $clconn = ldap_connect($ldapuri);
    if ( $fp && $clconn && !@$sitetag[$clhost] ) {
      fclose($fp);
      array_push($dsarray,$clconn);
      array_push($hnarray,$clhost);
      array_push($pnarray,$clport);
      array_push($dnarray,$basedn);
      @ldap_set_option($clconn, LDAP_OPT_NETWORK_TIMEOUT, $tout);
      $sitetag[$clhost] = 1; /* filtering tag */
      if ($debug==2) dbgmsg("$k - <i>$clhost:$clport </i>");
    } elseif ( $fp && $clconn && @$sitetag[$clhost] ) {
      fclose($fp);
      if ( $schema == "GLUE2"){
          // Add only the base option
          $index = array_keys($hnarray, $clhost);
          $dnarray[$index[0]] = DN_GLUE;
      } elseif ( $schema == "NG"){
          // Add only the base option
          $index = array_keys($hnarray, $clhost);
          $dnarray[$index[0]] = DN_LOCAL;
      } else {
          array_push($dsarray,$clconn);
          array_push($hnarray,$clhost);
          array_push($pnarray,$clport);
          array_push($dnarray,$basedn);
      }
    }
  }
  
  $nhosts = count($dsarray);
  if( $debug == 2 ) dbgmsg("<BR>".$nhosts.$errors["108"]."<br>");
  if ( !$nhosts ) {
    // NO SITES REPLY...
    $errno = "2";
    echo "<BR><font color=\"red\"><B>".$errors[$errno]."</B></font>\n";
    return $errno;
  }
  
  // Search all clusters and queues
  
  $ts1 = time();
    $srarray = @ldap_search($dsarray,$dnarray,$filter,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
  // If using the patched LDAP
  //$srarray = @ldap_search($dsarray,DN_LOCAL,$filter,$lim,0,0,$tlim,LDAP_DEREF_NEVER,$tout);
  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["109"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
  
  /*  
   *  $ts1 = time(); 
   *  $qsarray = @ldap_search($dsarray,DN_LOCAL,$qfilstr,$qlim,0,0,$tlim,LDAP_DEREF_NEVER);  
   *  // Fall back to a conventional LDAP
   *  //  if ( !count($qsrarray)) $qsarray = @ldap_search($dsarray,DN_LOCAL,$qfilstr,$qlim,0,0,$tlim,LDAP_DEREF_NEVER);  
   *  $ts2 = time(); if($debug) dbgmsg("<br><b>".$errors["110"]." (".($ts2-$ts1).$errors["104"].")</b><br>");
   */
  
  // Loop on clusters
  
  for ( $ids = 0; $ids < $nhosts; $ids++ ) {
    $entries  = array();
    $jentries = array();
    $gentries = array();
    $rowcont  = array();
    $sr = $srarray[$ids];
    $hn = $hnarray[$ids];
    $pn = $pnarray[$ids];
    $ds = $dsarray[$ids];
    $nr = @ldap_count_entries($ds,$sr);

    if ( !$sr || !$ds || !$nr ) {
      $error = ldap_error($ds);
      if ( $error == "Success" ) $error = $errors["3"];
      if ( $debug ) dbgmsg("<b><font color=\"red\">".$errors["111"]."$hn ($error)</font></b><br>");
      $sr = FALSE;
    }
    if ($ds && $sr) {
      
      $entries   = @ldap_get_entries($ds,$sr);
      $nclusters = $entries["count"];      /* Actually, cluster and queue blocks, 2+ */

      if ( !$nclusters ) {
    if ( $debug ) dbgmsg("<b>$hn</b>:".$errors["3"]."<br>");
    continue;
      }
      
      $nclu = 0;
      $nqueues = 0;
      $allqrun    = 0;
      $lrmsrun    = 0;
      $gridjobs   = 0;
      $allqueued  = 0;
      $gridqueued = 0;
      $lrmsqueued = 0;
      $prequeued  = 0;
      $totgridq   = 0;
      
      $toflag2 = FALSE;
      
      $stopflag = FALSE;
      
      for ($i=0; $i<$nclusters; $i++) {
    $curdn   = $entries[$i]["dn"];

        $preflength = strrpos($curdn,","); 
        $basedn  = substr($curdn,$preflength+1);
        $allbasedn  = strtolower(substr($curdn,$preflength-17));

        if ($basedn == DN_GLUE) {
      // extract objectclass name from DN -- shouldn't this be easier?
          $preflength = strpos($curdn,":");
          $preflength = strpos($curdn,":",$preflength+1);
      $object  = substr($curdn,$preflength+1,strpos($curdn,":",$preflength+1)-$preflength-1);
      if ($object=="ComputingService") {
 
        $dnparts = ldap_explode_dn($curdn,0);
            $endpointArray=explode(":",$dnparts[0]);

            $curname = $endpointArray[3];
        $curport = $pn;
      
        $curalias   = $entries[$i][GCLU_ANAM][0];
      
        // Manipulate alias: replace the string if necessary and cut off at 22 characters; strip HTML tags
        if (file_exists("cnvalias.inc")) include('cnvalias.inc');
        $curalias = strip_tags($curalias);
            //TODO: if alias empty, use endpoint FQDN (from endpointArray maybe?)
            if ( empty($curalias) ) {
                $curalias = "Undefined. FQDN is ".$curname;
            }
        if ( strlen($curalias) > 22 ) $curalias = substr($curalias,0,21) . ">";
        
            // TODO: probably remove QJOB, GLUE2 does not support it
            //$totqueued  = @($entries[$i][GCLU_QJOB][0]) ? $entries[$i][GCLU_QJOB][0] : 0; /* deprecated since 0.5.38 */
        $gmqueued   = @($entries[$i][GCLU_PQUE][0]) ? $entries[$i][GCLU_PQUE][0] : 0; /* new since 0.5.38 */
            $curtotjobs = @($entries[$i][GCLU_TJOB][0]) ? $entries[$i][GCLU_TJOB][0] : 0;
            $clstring   = popup("clusdes.php?host=$curname&port=$curport&schema=$schema",700,620,1,$lang,$debug);
      
        $nclu++;
      
      } elseif ($object=="ComputingManager") {
            $curtotcpu = @($entries[$i][GCLU_TCPU][0]) ? $entries[$i][GCLU_TCPU][0] : 0;
            if ( !$curtotcpu && $debug ) dbgmsg("<font color=\"red\"><b>$curname</b>".$errors["113"]."</font><br>");
            $curusedcpu = @($entries[$i][GCLU_UCPU][0]) ? $entries[$i][GCLU_UCPU][0] : -1; 

          } elseif ($object=="ComputingShare") {
      
        $qstatus     = $entries[$i][GQUE_STAT][0];
        if ( $qstatus != "production" )  $stopflag = TRUE;
        $allqrun    += @($entries[$i][GQUE_RUNG][0]) ? ($entries[$i][GQUE_RUNG][0]) : 0;
            // TODO: there is no gridjobs in GLUE2. Must be calculated as $allqrun-$lrmsrun (and adjusted against negative numbers), the latter has been added in settings.php. Test!
        //$gridjobs   += @($entries[$i][GQUE_GRUN][0]) ? ($entries[$i][GQUE_GRUN][0]) : 0;
            $lrmsrun += @($entries[$i][GQUE_LRUN][0]) ? ($entries[$i][GQUE_LRUN][0]) : 0;
            $gridjobs = $allqrun - $lrmsrun;
            if ( $gridjobs < 0 )  $gridjobs = 0;
        $gridqueued += @($entries[$i][GQUE_GQUE][0]) ? ($entries[$i][GQUE_GQUE][0]) : 0;
            // this below was deprecated in NG and I am not sure we want it back in GLUE2
        //$allqueued  += @($entries[$i][GQUE_QUED][0]) ? ($entries[$i][GQUE_QUED][0]) : 0; /* deprecated since 0.5.38 */
        $lrmsqueued += @($entries[$i][GQUE_LQUE][0]) ? ($entries[$i][GQUE_LQUE][0]) : 0; /* new since 0.5.38 */
        $prequeued  += @($entries[$i][GQUE_PQUE][0]) ? ($entries[$i][GQUE_PQUE][0]) : 0; /* new since 0.5.38 */
      
        $nqueues++;
      
      } elseif ($object=="Location") {
            $dnparts = ldap_explode_dn($curdn,0);
            $endpointArray=explode(":",$dnparts[0]);

            $curname = $endpointArray[3];
            $curport = $pn;

            // Country name massaging
            $vo = guess_country($curname,$entries[$i][GCLU_ZIPC][0]);
            if ($debug==2) dbgmsg("<i>$ids: <b>$curname</b>".$errors["112"]."$vo</i><br>");
            $vostring = $_SERVER['PHP_SELF']."?display=vo=$vo";
      if ( $lang != "default") $vostring .= "&lang=".$lang;
      if ( $debug ) $vostring .= "&debug=".$debug;
            $country  = $vo;
            if ( $yazyk !== "en" ) $country = $strings["tlconvert"][$vo];
            $country_content = "<a href=\"$vostring\"><img src=\"./mon-icons/$vo.png\" title=\"".$errors["312"]."$country \" alt=\"".$errors["312"]."\" height=\"10\" width=\"16\" border=\"0\">&nbsp;<i><b>".$country."</b></i></a>&nbsp;";
            if (!in_array($country_content,$rowcont)){
                $rowcont[] = $country_content;
            }
          } //TODO: Add Contact and AdminDomain objects
        // TODO: I think this below should be basedn otherwise this chunk is run even for GLUE2 overriding all the changes made above... the if tree must be revised. I think there is a lot of overdoing here.
    } elseif ($allbasedn == DN_LOCAL) {
      // check if it is a site or a job; count
      $preflength = strpos($curdn,"-");
      $object  = substr($curdn,$preflength+1,strpos($curdn,"-",$preflength+1)-$preflength-1);
      if ($object=="cluster") {
      
        $dnparts = ldap_explode_dn($curdn,0);
        $curname = substr(strstr($dnparts[0],"="),1);
        $curport = $pn;
      
        // Country name massaging
      
      $zip = "";
        if ( !empty($entries[$i][CLU_ZIPC][0]) ) $zip = $entries[$i][CLU_ZIPC][0];
      $vo = guess_country($curname,$zip);
        if ($debug==2) dbgmsg("<i>$ids: <b>$curname</b>".$errors["112"]."$vo</i><br>");
        $vostring = $_SERVER['PHP_SELF']."?display=vo=$vo";
      if ( $lang != "default") $vostring .= "&lang=".$lang;
      if ( $debug ) $vostring .= "&debug=".$debug;
        $country  = $vo;
        if ( $yazyk !== "en" ) $country = $strings["tlconvert"][$vo];
      
        $rowcont[] = "<a href=\"$vostring\"><img src=\"./mon-icons/$vo.png\" title=\"".$errors["312"]."$country \" alt=\"".$errors["312"]."\" height=\"10\" width=\"16\" border=\"0\">&nbsp;<i><b>".$country."</b></i></a>&nbsp;";
      
        $curtotcpu = @($entries[$i][CLU_TCPU][0]) ? $entries[$i][CLU_TCPU][0] : 0;
        if ( !$curtotcpu && $debug ) dbgmsg("<font color=\"red\"><b>$curname</b>".$errors["113"]."</font><br>");
      
        $curalias   = $entries[$i][CLU_ANAM][0];
      
        // Manipulate alias: replace the string if necessary and cut off at 22 characters; strip HTML tags
        if (file_exists("cnvalias.inc")) include('cnvalias.inc');
        $curalias = strip_tags($curalias);
        if ( strlen($curalias) > 22 ) $curalias = substr($curalias,0,21) . ">";
      
        $curtotjobs = @($entries[$i][CLU_TJOB][0]) ? $entries[$i][CLU_TJOB][0] : 0;
        $curusedcpu = @($entries[$i][CLU_UCPU][0]) ? $entries[$i][CLU_UCPU][0] : -1;
        $totqueued  = @($entries[$i][CLU_QJOB][0]) ? $entries[$i][CLU_QJOB][0] : 0; /* deprecated since 0.5.38 */
        $gmqueued   = @($entries[$i][CLU_PQUE][0]) ? $entries[$i][CLU_PQUE][0] : 0; /* new since 0.5.38 */
      $clstring   = popup("clusdes.php?host=$curname&port=$curport",700,620,1,$lang,$debug);
      
        $nclu++;
      
      } elseif ($object=="queue") {
      
        $qstatus     = $entries[$i][QUE_STAT][0];
        if ( $qstatus != "active" )  $stopflag = TRUE;
        $allqrun    += @($entries[$i][QUE_RUNG][0]) ? ($entries[$i][QUE_RUNG][0]) : 0;
            $gridjobs   += @($entries[$i][QUE_GRUN][0]) ? ($entries[$i][QUE_GRUN][0]) : 0;
        $gridqueued += @($entries[$i][QUE_GQUE][0]) ? ($entries[$i][QUE_GQUE][0]) : 0;
        $allqueued  += @($entries[$i][QUE_QUED][0]) ? ($entries[$i][QUE_QUED][0]) : 0; /* deprecated since 0.5.38 */
        $lrmsqueued += @($entries[$i][QUE_LQUE][0]) ? ($entries[$i][QUE_LQUE][0]) : 0; /* new since 0.5.38 */
        $prequeued  += @($entries[$i][QUE_PQUE][0]) ? ($entries[$i][QUE_PQUE][0]) : 0; /* new since 0.5.38 */
      
        $nqueues++;
      
      }

        }
      }

      if ( !$nclu && $nqueues ) {
    if ( $debug ) dbgmsg("<b>$hn</b>:".$errors["3"].": ".$errors["111"].$errors["410"]."<br>");
    continue;
      }

      if ( $nclu > 1 && $debug ) dbgmsg("<b><font color=\"blue\">$hn</font></b>:".$errors["3"].": $nclu ".$errors["406"]."<br>");

      if (!$nqueues) $toflag2 = TRUE;

      if ($debug==2 && $prequeued != $gmqueued) dbgmsg("<i><font color=\"blue\">$curname:</font></i> cluster-prelrmsqueued != sum(queue-prelrmsqueued)");
      
      $allrun    = ($curusedcpu < 0) ? $allqrun             : $curusedcpu; 
      if ($gridjobs > $allrun) $gridjobs = $allrun;
      
      /*    For versions < 0.5.38:
       *   Some Grid jobs are counted towards $totqueued and not towards $allqueued 
       *   (those in GM), so $totqueued - $allqueued = $gmqueued,
       *   and $truegridq = $gmqueued + $gridqueued
       *   and $nongridq  = $totqueued - $truegridq == $allqueued - $gridqueued
       *   hence $truegridq = $totqueued - $nongridq
       */
      $nongridq  = ($totqueued) ? $allqueued - $gridqueued : $lrmsqueued;
      $truegridq = ($totqueued) ? $totqueued - $nongridq : $gridqueued + $prequeued;
      // temporary hack:
      //      $truegridq = $gridqueued;
      //
      $formtgq    = sprintf(" s",$truegridq);
      $formngq    = sprintf("\&nbsp\;s",$nongridq);
      $localrun  = $allrun - $gridjobs;
      $gridload  = ($curtotcpu > 0)  ? $gridjobs/$curtotcpu : 0;
      $clusload  = ($curtotcpu > 0)  ? $allrun/$curtotcpu   : 0;
      $tstring   = urlencode("$gridjobs+$localrun");
      $jrstring  = popup("jobstat.php?host=$curname&port=$curport&status=Running&jobdn=all",600,500,2,$lang,$debug);
      $jqstring  = popup("jobstat.php?host=$curname&port=$curport&status=Queueing&jobdn=all",600,500,2,$lang,$debug);
      if ( $schema == "GLUE2"){
        $jrstring  = popup("jobstat.php?host=$curname&port=$curport&status=Running&jobdn=all&schema=$schema",600,500,2,$lang,$debug);
        $jqstring  = popup("jobstat.php?host=$curname&port=$curport&status=Queueing&jobdn=all&schema=$schema",600,500,2,$lang,$debug);
      }
      
      if ( $toflag2 ) {
    $tstring .= " (no queue info)"; // not sure if this is localizeable at all
      } elseif ( $stopflag ) {
    $tstring .= " (queue inactive)"; // not sure if this is localizeable at all
      }

      // Add a cluster row
      
      $rowcont[] = "<a href=\"$clstring\">&nbsp;<b>$curalias</b></a>";
      $rowcont[] = "$curtotcpu";
      if ( $curtotcpu ) {
    $rowcont[] = "<a href=\"$jrstring\"><img src=\"./mon-icons/icon_bar.php?x=".$clusload."&xg=".$gridload."&y=13&text=".$tstring."\" vspace=\"2\" hspace=\"3\" border=\"0\" title=\"$gridjobs".$errors["313"]."$localrun".$errors["314"]."\" alt=\"$gridjobs+$localrun\" width=\"200\" height=\"13\"></a>";
      } else {
    $rowcont[] = "<a href=\"$jrstring\"><img src=\"./mon-icons/spacer.gif\" vspace=\"2\" hspace=\"3\" border=\"0\" title=\"$gridjobs".$errors["313"]."$localrun".$errors["314"]."\"  alt=\"$gridjobs+$localrun\" width=\"200\" height=\"13\"></a>";
      }
      //      $rowcont[] = "<a href=\"$jqstring\">$totqueued</a>";
      
      $rowcont[] = "<a href=\"$jqstring\"><b>$truegridq</b></a>+$nongridq";
      // Not adding anymore, cache instead
      //    $ctable->addrow($rowcont);
      $tcont[] = $rowcont;
      $rowcont = array ();
      
    }

  }
  
  // Dump the collected table
  
  cache_table($cachefile,$tcont);
}

// HTML table initialization

$ctable = new LmTableSp($module,$toppage->$module);

// Sort
/** possible ordering keywords:
 *    country - sort by country, default
 *    cpu     - sort by advertised CPU number
 *    grun    - sort by number of running Grid jobs
 */
$ostring = "comp_by_".$order;

usort($tcont,$ostring);

$nrows = count($tcont);

$votolink    = array();
$affiliation = array();

foreach ( $tcont as $trow ) {
  $vo = $trow[0];
  $vo = substr(stristr($vo,"./mon-icons/"),12);
  $vo = substr($vo,0,strpos($vo,"."));
  if ( !in_array($vo,$votolink) ) $votolink[]=$vo;
  array_push($affiliation,$vo);
}

$affcnt = array_count_values($affiliation);

$prevvo = "boo";

$sumcpu        = 0;
$sumgridjobs   = 0;
$sumlocljobs   = 0;
$sumclusters   = 0;
$sumgridqueued = 0;
$sumloclqueued = 0;
//$sumqueued     = 0;

// actual loop

foreach ( $tcont as $trow ) {

  $gridjobs = $trow[3];
  $gridjobs = substr(stristr($gridjobs,"alt=\""),5);
  $gridjobs = substr($gridjobs,0,strpos($gridjobs,"+"));

  $localrun = $trow[3];
  $localrun = substr(stristr($localrun,"+"),1);
  $localrun = substr($localrun,0,strpos($localrun,"\" w"));

  $truegridq = $trow[4];
  $truegridq = substr(stristr($truegridq,"<b>"),3);
  $truegridq = substr($truegridq,0,strpos($truegridq,"</b>"));

  $nongridq = $trow[4];
  $nongridq = substr(stristr($nongridq,"+"),1);

  $vo = $trow[0];
  $vo = substr(stristr($vo,"./mon-icons/"),12);
  $vo = substr($vo,0,strpos($vo,"."));
  if ( @$showvo && $showvo != $vo ) continue;

  $sumcpu        += $trow[2];
  $sumgridjobs   += $gridjobs;
  $sumlocljobs   += $localrun;
  $sumgridqueued += $truegridq;
  $sumloclqueued += $nongridq;
  //  $sumqueued     += $totqueued;
  $sumclusters ++;

  if ( $vo != $prevvo && $order == "country" ) { // start new country rowspan
    $prevvo   = $vo;
    $vostring = $trow[0];
    $ctable->addspacer("#000099");
    $ctable->rowspan( $affcnt[$vo], $vostring, "#FFF2DF" );
    $tcrow = array_shift($trow);
    $ctable->addrow($trow);
  } else {
    if ( $order == "country" ) $tcrow = array_shift($trow);
    $ctable->addrow($trow);
  }
  
}

$tcont = array();

$ctable->addspacer("#990000");
$rowcont[] = "<b><i>".$errors["405"]."</i></b>";
$rowcont[] = "<b><i>$sumclusters".$errors["406"]."</i></b>";
$rowcont[] = "<b><i>$sumcpu</i></b>";
$rowcont[] = "<b><i>$sumgridjobs + $sumlocljobs</i></b>";
$rowcont[] = "<b><i>$sumgridqueued + $sumloclqueued</i></b>";
//  $rowcont[] = "<b><i>$sumqueued</i></b>";
$ctable->addrow($rowcont, "#ffffff");
$ctable->close();

// To change language, link back to ALL

$linkback = $_SERVER['PHP_SELF'];
if ( $debug ) {
  $linkback .= "?debug=".$debug;
  $separator = "&";
 } else {
  $separator = "?";
 }

// Show flags if only one country is chosen

if ( @$showvo ) {
  echo "<br><nobr>\n";
  foreach ( $votolink as $volink ) {
    $vostring  = $_SERVER['PHP_SELF']."?display=vo=$volink";
    if ( $lang != "default" ) $vostring .= "&lang=".$lang;
    if ( $debug ) $vostring .= "&debug=".$debug;
    $voimage   = "<img src=\"./mon-icons/$volink.png\" title=\"".$errors["312"]."$volink\" alt=\"".$errors["312"]."\" height=\"10\" width=\"16\" border=\"0\">";
    echo "<a href=\"$vostring\">$voimage</a>&nbsp;&nbsp;";
  }
  if ( $lang != "default") $linkall = $linkback.$separator."lang=".$lang;
  echo "<a href=\"".$linkall."\"><b>".$errors["409"]."</b></a><BR>\n";  // Show ALL
  echo "</nobr>\n";
 } else {

  // Show languages
  
  $translations = scandir(getcwd()."/lang");
  echo "<center><br><nobr>\n";
  foreach ( $translations as $transfile ) {
    $twoletcod = substr($transfile,0,2);
    if ( stristr($transfile,".") == ".inc" && $twoletcod != "us" ) {
      $linklang = $linkback.$separator."lang=".$twoletcod;
      echo "<a href=\"".$linklang."\">$twoletcod</a>&nbsp;&nbsp;";
    }
  }
  echo "</nobr><br></center>\n";
 }

return 0;

// Done

$toppage->close();

?>
