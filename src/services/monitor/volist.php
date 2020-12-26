<?php
 
// Author: oxana.smirnova@hep.lu.se

/**
 * Static list of known VOs, with dynamic members count
 */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');
require_once('lmtable.inc');
require_once('comfun.inc');
require_once('toreload.inc');
require_once('ldap_purge.inc');

$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);
$debug   = @$_GET["debug"];
if ( !$debug )  $debug    = 0;

// Setting up the page itself

$toppage = new LmDoc("volist");
$toptitle = $toppage->title;
$module   = &$toppage->module;
$strings  = &$toppage->strings;
$errors   = &$toppage->errors;

// Header table

$toppage->tabletop("<font face=\"Verdana,Geneva,Arial,Helvetica,sans-serif\">".$toptitle."<br><br></font>");

// The main function

$vos = array (
	      array (
		     "name"   => "NorduGrid members",
		     "server" => "grid-vo.nordugrid.org",
		     "port"   => "389",
		     "dn"     => "ou=people,dc=nordugrid,dc=org"
		     ),
	      array (
		     "name"   => "NorduGrid guests",
		     "server" => "https://www.pdc.kth.se/grid/swegrid-vo",
		     "port"   => "",
		     "dn"     => ""
		     ),
	      array (
		     "name"   => "NorduGrid developers",
		     "server" => "http://www.nordugrid.org",
		     "port"   => "",
		     "dn"     => "",
		     "group"  => "developers.dn"
		     ),
	      array (
		     "name"   => "NorduGrid tutorials",
		     "server" => "grid-vo.nordugrid.org",
		     "port"   => "389",
		     "dn"     => "ou=tutorial,dc=nordugrid,dc=org"
		     ),
	      array (
		     "name"   => "ATLAS test users (SWEGRID)",
		     "server" => "https://www.pdc.kth.se",
		     "port"   => "",
		     "dn"     => "",
		     "group"  => "grid/swegrid-vo/vo.atlas-testusers-vo"
		     ),
	      /*
	      array (
		     "name"   => "NorduGrid services",
		     "server" => "grid-vo.nordugrid.org",
		     "port"   => "389",
		     "dn"     => "ou=services,dc=nordugrid,dc=org"
		     ),
	      */
	      array (
		     "name"   => "BaBar",
		     "server" => "babar-vo.gridpp.ac.uk",
		     "port"   => "389",
		     "dn"     => "ou=babar,dc=gridpp,dc=ac,dc=uk"
		     ),
	      array (
		     "name"   => "EDG ALICE",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=alice,dc=eu-datagrid,dc=org"
		     ),
	      array (
		     "name"   => "EDG ATLAS",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=atlas,dc=eu-datagrid,dc=org"
		     ),
	      array (
		     "name"   => "LCG ATLAS",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=atlas,dc=eu-datagrid,dc=org",
		     "group"  => "ou=lcg1"
		     ),	      
	      array (
		     "name"   => "EDG CMS",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=cms,dc=eu-datagrid,dc=org"
		     ),
	      array (
		     "name"   => "EDG LHC-B",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=lhcb,dc=eu-datagrid,dc=org"
		     ),
	      array (
		     "name"   => "EDG D0",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=dzero,dc=eu-datagrid,dc=org",
		     "group"  => "ou=testbed1"
		     ),
	      array (
		     "name"   => "EDG Earth Observation",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=earthob,dc=eu-datagrid,dc=org"
		     ),
	      array (
		     "name"   => "EDG Genomics",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=biomedical,dc=eu-datagrid,dc=org",
		     "group"  => "ou=genomics"
		     ),
	      array (
		     "name"   => "EDG Medical Imaging",
		     "server" => "grid-vo.nikhef.nl",
		     "port"   => "389",
		     "dn"     => "o=biomedical,dc=eu-datagrid,dc=org",
		     "group"  => "ou=medical imaging"
		     ),
	      array (
		     "name"   => "EDG ITeam",
		     "server" => "marianne.in2p3.fr",
		     "port"   => "389",
		     "dn"     => "o=testbed,dc=eu-datagrid,dc=org",
		     "group"  => "ou=ITeam"
		     ),
	      array (
		     "name"   => "EDG TSTG",
		     "server" => "marianne.in2p3.fr",
		     "port"   => "389",
		     "dn"     => "o=testbed,dc=eu-datagrid,dc=org",
		     "group"  => "ou=TSTG"
		     ),
	      array (
		     "name"   => "EDG Tutorials",
		     "server" => "marianne.in2p3.fr",
		     "port"   => "389",
		     "dn"     => "o=testbed,dc=eu-datagrid,dc=org",
		     "group"  => "ou=EDGtutorial"
		     ),
	      array (
		     "name"   => "EDG WP6",
		     "server" => "marianne.in2p3.fr",
		     "port"   => "389",
		     "dn"     => "o=testbed,dc=eu-datagrid,dc=org",
		     "group"  => "ou=wp6"
		     )
	      );
	      
$votable = new LmTableSp($module,$toppage->$module);
$rowcont = array ();
foreach ( $vos as $contact ) {
  $server    = $contact["server"];
  $port      = $contact["port"];
  $dn        = $contact["dn"];
  $group     = "";
  if ( !empty($contact["group"]) ) $group = $contact["group"];

  $nusers    = 0;

  if ( $dn ) { // open ldap connection
    $ldapuri = "ldap://".$server.":".$port;
    $ds = ldap_connect($ldapuri);
    if ($ds) {
      if ( $group ) {
        $newfilter = "(objectclass=*)";
        $newdn     = $group.",".$dn;
        $newlim    = array("dn","member");
        $sr        = @ldap_search($ds,$newdn,$newfilter,$newlim,0,0,10,LDAP_DEREF_NEVER);
        if ($sr) {
          $groupdesc = @ldap_get_entries($ds,$sr);
          $nusers    = $groupdesc[0]["member"]["count"];
        }
      } else {
        $sr = @ldap_search($ds,$dn,"(objectclass=organizationalPerson)",array("dn"),0,0,10,LDAP_DEREF_NEVER);
        if ($sr) $nusers = @ldap_count_entries($ds,$sr);
      }
    }
    $vostring  = popup("vo-users.php?host=$server&port=$port&vo=$dn&group=$group",750,300,6,$lang,$debug);
  } else {
    $url = $server."/".$group;
    $users = file($url);
    $nusers = 0;
    if ( !empty($users) ) $nusers = count($users);
    $vostring = popup($url,750,300,6,$lang,$debug);
  }
  
  $rowcont[] = "<a href=\"$vostring\">".$contact["name"]."</a>";
  $rowcont[] = $nusers;
  $rowcont[] = $server;
  
  $votable->addrow($rowcont);
  $rowcont = array ();
}

$votable->close();

$toppage->close();

     /*

group http://www.nbi.dk/~waananen/ngssc2003.txt

### Datagrid VO Groups and their user mappings
     */
?>