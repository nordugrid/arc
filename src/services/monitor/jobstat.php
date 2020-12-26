<?php
 
// Author: oxana.smirnova@hep.lu.se
/**
 * Retrieves job-specific information from a given NorduGrid queue
 * Uses LDAP functions of PHP
 * 
 * Author: O.Smirnova (May 2002)
 * inspired by the LDAPExplorer by T.Miao
 * 
 * input:
 * host (default: grid.quark.lu.se)
 * port (default: 2135)
 * status (Running|Preparing|whatever)
 * jobdn (all|DN of a single job)
 * 
 * output:
 * an HTML table, containing:
 * per each cluster:
 * - list of running or queued jobs
 */
  

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_nice_dump.inc');

// getting parameters

$host   = @( $_GET["host"] )   ? $_GET["host"]   : "quark.hep.lu.se";
$port   = @( $_GET["port"] )   ? $_GET["port"]   : 2135;
$status = @( $_GET["status"] ) ? $_GET["status"] : "Running";
$jobdn  = @( $_GET["jobdn"] )  ? $_GET["jobdn"]  : "all";
$debug  = @( $_GET["debug"] )  ? $_GET["debug"]  : 0;
$schema = @( $_GET["schema"] ) ? $_GET["schema"] : "NG";
$lang   = @$_GET["lang"];
if ( !$lang )    $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

// Setting up the page itself

$toppage = new LmDoc("jobstat",$host);
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;

// Header table
  
$titles   = explode(":",$toptitle); // two alternative titles, separated by column

if ($jobdn=="all") {
  $clstring = popup("clusdes.php?host=$host&port=$port&schema=$schema",700,620,1,$lang,$debug);
  $gtitle   = "<b><i>".$titles[0]." <a href=\"$clstring\">$host</a></i></b>";
} else {
  $jobdn     = rawurldecode($jobdn);
  $jobdn     = preg_replace("/\"/","",$jobdn);
  $dn_pieces = ldap_explode_dn($jobdn,1);
  $jobgid    = $dn_pieces[0];
  $gtitle    = "<b>".$titles[1].": <i>$jobgid</i></b>";
}

$toppage->tabletop("",$gtitle);

// Arrays defining the attributes to be returned

$lim = array( "dn", JOB_NAME, JOB_EQUE, JOB_GOWN, JOB_STAT, JOB_USET, JOB_SUBM, JOB_CPUS );

// ldapsearch filter string for jobs

$filstr="(objectclass=".OBJ_AJOB.")";

if ( $schema == "GLUE2") {
  $lim = array( "dn", GJOB_NAME, GJOB_EQUE, GJOB_GOWN, GJOB_STAT, GJOB_USET, GJOB_SUBM, GJOB_CPUS );
  $filstr="(objectclass=".GOBJ_AJOB.")";
}

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 15;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// Establish connection to the requested LDAP server

$ldapuri = "ldap://".$host.":".$port;
$ds    = ldap_connect($ldapuri);
$bdn   = DN_LOCAL;
$topdn = DN_GLOBL;
if ( $schema == "GLUE2") {
  $bdn   = DN_GLUE;
  if ($jobdn != "all") $bdn = "";
}
if ($ds) {
  
  // Single job info dump and quit
  
  if ($jobdn != "all") {
    //    $basedn = explode("Mds",$jobdn);
    $basedn = preg_split("/mds/i",$jobdn);
    $locdn  = $basedn[0].$bdn;
    $thisdn = ldap_nice_dump($strings,$ds,$locdn);
    ldap_close($ds);
    return 0;
  }
  
  // Loop over all the jobs
  
  $sr = @ldap_search($ds,$bdn,$filstr,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
  // Fall back to conventional LDAP
  //  if (!$sr) $sr = @ldap_search($ds,$bdn,$filstr,$lim,0,0,$tlim,LDAP_DEREF_NEVER);
  
  if ($sr) {
    
    // If search returned, check that there are valid entries
      
    $nmatch = ldap_count_entries($ds,$sr);
    if ($nmatch > 0) {
        
      // HTML table initialisation
        
      $jtable = new LmTable($module,$toppage->$module);
        
      // If there are valid entries, tabulate results
        
      $entries = ldap_get_entries($ds,$sr);
      $njobs   = $entries["count"];

      define("CMPKEY",JOB_SUBM);
      if ( $schema == "GLUE2")  define("CMPKEY",GJOB_SUBM);
      usort($entries,"ldap_entry_comp");
        
      // loop on jobs
        
      $jcount = 0;
      for ($i=1; $i<$njobs+1; $i++) {
	$jdn     = rawurlencode($entries[$i]["dn"]);
	$curstat = $entries[$i][JOB_STAT][0];
        if ( $schema == "GLUE2") {
          $curstat = $entries[$i][GJOB_STAT][0];
        }
          
	/*
	 * The following flags may need an adjustment, 
	 * depending on the Job Status provider
	 */

	// Running job: statail == "R" or "run"
	//          $statail = substr($curstat,-3);
	$statail = substr(strstr($curstat,"INLRMS:"),7);
	$statail = trim($statail);

	// Queued job: stahead != "FIN" && statail != "R" and "run" etc
	$stahead = substr($curstat,0,3);
          
	$flagrun = ( $status  == "Running" && 
		     ( $statail == "R" ||  /* PBS */
		       $statail == "S" ||  /* suspended by Condor */
		       $statail == "run" ) /* easypdc */
		     );
	$flagque = ( $status  != "Running" &&
		     $statail != "R" &&
		     $statail != "S" &&
		     $statail != "run" &&
		     $stahead != "FIN"     &&
		     $stahead != "FAI"     &&
		     $stahead != "EXE"     &&
		     $stahead != "KIL"     &&
		     $stahead != "DEL" );
          
	/* No changes necessary below */
          
	$flagact = ($flagrun || $flagque)?1:0;
	if ($flagact == 1 || $status == "All" ) {
          if ( $schema == "GLUE2") {
	    $uname    = $entries[$i][GJOB_GOWN][0];
	    $encuname = rawurlencode($uname);
	    $uname    = addslashes($uname); // In case $uname contains escape characters
	    $family   = cnvname($uname, 2);

	    $jname   = htmlentities($entries[$i][JOB_NAME][0]);
	    $jobname = ($entries[$i][GJOB_NAME][0]) ? $jname : "<font color=\"red\">N/A</font>";
	    $queue   = ($entries[$i][GJOB_EQUE][0]) ? $entries[$i][GJOB_EQUE][0] : "";
	    $time    = ($entries[$i][GJOB_USET][0]) ? $entries[$i][GJOB_USET][0] : "";
	    $ncpus   = ($entries[$i][GJOB_CPUS][0]) ? $entries[$i][GJOB_CPUS][0] : "";
	    $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jdn&schema=$schema",750,430,4,$lang,$debug);
	    $quewin  = popup("quelist.php?host=$host&port=$port&qname=$queue&schema=$schema",750,430,6,$lang,$debug);
	    $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname&schema=$schema",700,500,5,$lang,$debug);
          } else {
            $uname    = $entries[$i][JOB_GOWN][0];
            $encuname = rawurlencode($uname);
            $uname    = addslashes($uname); // In case $uname contains escape characters
            $family   = cnvname($uname, 2);

            $jname   = htmlentities($entries[$i][JOB_NAME][0]);
            $jobname = ($entries[$i][JOB_NAME][0]) ? $jname : "<font color=\"red\">N/A</font>";
            $queue   = ($entries[$i][JOB_EQUE][0]) ? $entries[$i][JOB_EQUE][0] : "";
            $time    = ($entries[$i][JOB_USET][0]) ? $entries[$i][JOB_USET][0] : "";
            $ncpus   = ($entries[$i][JOB_CPUS][0]) ? $entries[$i][JOB_CPUS][0] : "";
            $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jdn",750,430,4,$lang,$debug);
            $quewin  = popup("quelist.php?host=$host&port=$port&qname=$queue",750,430,6,$lang,$debug);
            $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname",700,500,5,$lang,$debug);
          }

	  $jcount++;

	  // filling the table
            
	  $jrowcont[] = "$jcount&nbsp;<a href=\"$newwin\">$jobname</a>";
	  $jrowcont[] = "<a href=\"$usrwin\">$family</a>";
	  $jrowcont[] = "$curstat";
	  $jrowcont[] = "$time";
	  $jrowcont[] = "<a href=\"$quewin\">$queue</a>";
	  $jrowcont[] = "$ncpus";
	  $jtable->addrow($jrowcont);
	  $jrowcont = array ();
	}
      }
      if ($jcount == 0) $jtable->adderror("<b>".$errors["4"].": ".$status."</b>");
      $jtable->close();
    }
    else {
      echo "<br><font color=\"red\"><b>".$errors["4"]."</b>".$errors["7"]."</font>\n";
    }
  }
  else {
    echo "<br><font color=\"red\"><b>".$errors["4"]."</b>".$errors["7"]."</font>\n";
  }
  $entries = array();
  @ldap_free_result($sr);
  ldap_close($ds);
  return 0;
}
else {
  $errno = "6";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  return $errno;
}

// Done

$toppage->close();

?>
