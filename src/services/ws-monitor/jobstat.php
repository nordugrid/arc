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

include_once 'db.php';
set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('cnvtime.inc');
require_once('cnvname.inc');
require_once('comfun.inc');
require_once('toreload.inc');

// getting parameters

$host   = ( $_GET["host"] )   ? $_GET["host"]   : "quark.hep.lu.se";
$port   = ( $_GET["port"] )   ? $_GET["port"]   : 2135;
$status = ( $_GET["status"] ) ? $_GET["status"] : "Running";
$jobdn  = ( $_GET["jobdn"] )  ? $_GET["jobdn"]  : "all";
$debug  = ( $_GET["debug"] )  ? $_GET["debug"]  : 0;

// Setting up the page itself

$toppage = new LmDoc("jobstat",$host);
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;
$cert     = &$toppage->cert;

// Header table

$titles   = explode(":",$toptitle); // two alternative titles, separated by column

if ($jobdn=="all") {
  $clstring = popup("clusdes.php?host=$host&port=$port",700,620,1);
  $gtitle   = "<b><i>".$titles[0]." <a href=\"$clstring\">$host</a></i></b>";
} else {
  $jobdn     = rawurldecode($jobdn);
  $jobdn     = preg_replace("/\"/","",$jobdn);
  $jobgid    = $jobdn;
  $gtitle    = "<b>".$titles[1].": <i>$jobgid</i></b>";
}

$toppage->tabletop("",$gtitle);

// Arrays defining the attributes to be returned

$lim = array( "dn", JOB_NAME, JOB_EQUE, JOB_GOWN, JOB_STAT, JOB_USET, JOB_SUBM, JOB_CPUS );

if ( $debug ) {
  ob_end_flush();
  ob_implicit_flush();
}

$tlim = 15;
$tout = 20;
if( $debug ) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["101"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><br>");

// Establish connection to the requested information server

$type = 'DB';
$isis = new $type($cert,array());

$bdn   = DN_LOCAL;
$topdn = DN_GLOBL;
if ($isis) {

  // Single job info dump and quit

  if ($jobdn != "all") {
    $basedn = explode("Mds",$jobdn);
    $basedn = preg_split("/mds/i",$jobdn);
    $locdn  = $basedn[0].$bdn;
    $info = $isis->cluster_info($host);
    $jobs = $info->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity;
    $actual_job = NULL;
    foreach ($jobs as $job) {
        if ($job->ID == $jobdn) $actual_job = $job;
    }
    $isis->xml_nice_dump($actual_job, "job", $debug);
    $toppage->close();
    return 0;
  }

  // Loop over all the jobs

  $sr = $info;

  if ($sr) {

    // If search returned, check that there are valid entries

    //$nmatch = ldap_count_entries($ds,$sr);
//TODO::
    $nmatch = 1;
    if ($nmatch > 0) {

      // HTML table initialisation

      $jtable = new LmTable($module,$toppage->$module);

      // If there are valid entries, tabulate results

      $jobs = $sr->Domains->AdminDomain->Services->ComputingService->ComputingEndpoint->ComputingActivities->ComputingActivity;
      $njobs  = count($jobs);

      define("CMPKEY",JOB_SUBM);
//      usort($entries,"ldap_entry_comp");

      // loop on jobs
      $jcount = 0;
      for ($i=0; $i<$njobs; $i++) {
          $jdn     = rawurlencode((string)$jobs[$i]->{JOB_GOWN});
          $curstat = $jobs[$i]->{JOB_STAT};
          /*
           * The following flags may need an adjustment,
           * depending on the Job Status provider
           */

          // Running job: statail == "R" or "run"
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
            $uname    = (string)$jobs[$i]->{JOB_GOWN}->{0};
            $encuname = rawurlencode($uname);
            $uname    = addslashes($uname); // In case $uname contains escape characters
            $family   = cnvname($uname, 2);
            $jname   = htmlentities($jobs[$i]->{JOB_NAME});
            $jobname = ($jobs[$i]->{JOB_NAME}) ? $jname : "<font color=\"red\">N/A</font>";
            $queue   = ($jobs[$i]->{JOB_EQUE}) ? $jobs[$i]->{JOB_EQUE} : "";
            $time    = ($jobs[$i]->{JOB_USET}) ? $jobs[$i]->{JOB_USET} : "";
            $ncpus   = ($jobs[$i]->{JOB_CPUS}) ? $jobs[$i]->{JOB_CPUS} : "";
            $newwin  = popup("jobstat.php?host=$host&port=$port&status=$status&jobdn=$jdn",750,430,4);
            $quewin  = popup("quelist.php?host=$host&port=$port&qname=$queue",750,430,6);
            $usrwin  = popup("userlist.php?bdn=$topdn&owner=$encuname",700,500,5);

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
          if ($jcount == 0) {
             $jtable->adderror("<b>".$errors["4"].": ".$status."</b>");
             break;
          }
      }

      $jtable->close();
    }
    else {
      echo "<br><font color=\"red\"><b>".$errors["4"]."</b>".$errors["7"]."</font>\n";
    }
  }
  else {
    echo "<br><font color=\"red\"><b>".$errors["4"]."</b>".$errors["7"]."</font>\n";
  }
  $toppage->close();
  return 0;
}
else {
  $errno = "6";
  echo "<br><font color=\"red\"><b>".$errors[$errno]."</b></font>\n";
  $toppage->close();
  return $errno;
}

// Done

$toppage->close();

?>
