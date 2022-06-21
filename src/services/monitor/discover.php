<?php

// Author: oxana.smirnova@hep.lu.se

/**
 * @desc Lists attributes specific for an object for consecutive search
 */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('settings.inc');
require_once('headfoot.inc');

$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

$toppage = new LmDoc("attlist");
define("TOPTIT",$toppage->title);
$strings  = &$toppage->strings;
$giislist = &$toppage->giislist;
$isattr   = &$toppage->isattr;
$errors   = &$toppage->errors;

require_once('attlist.inc');

$itself  = $_SERVER["PHP_SELF"];
$schema = @$_GET["schema"];
if (!$schema) $schema = "NG";
$itself = $itself."?schema=".$schema;

$ifsub = $_POST["submit"] ? TRUE : FALSE ;
$ifsel = $_POST["select"] ? TRUE : FALSE ;

echo "<div style={font-family:arial,helvetica,sans-serif;margin:30px;padding:10px;background-color:#ccccff;}>\n";

if ( $ifsub ) {

  // Call attributes list function for all selected arguments

  $request = $_POST;
  
  $attributes = array ();
  $signs      = array ();
  $filters    = array ();

  $attributes = $request["attributes"];
  $signs      = $request["signs"];
  $filters    = $request["filters"];
  $thething   = $request["scope"];
  if ( $thething == "job"     || 
       $thething == "queue"   || 
       $thething == "authuser" ) $thething = "cluster";

  //  $attwin = popup("attlist.php?attribute=$encatt",650,300,7,$lang,$debug);

  //TODO: change thething to object class, or it will never work for GLUE2. 
  //      alternative: keep job queue authuser and do guessing for GLUE2.
  do_attlist($thething,$attributes,$signs,$filters,$strings,$giislist,$archery_list,$schema);

  echo " <div align=\"center\"><a href=\"javascript:history.go(-1)\"><img src=\"./mon-icons/icon_back.php\" vspace=\"5\" hspace=\"1\" border=\"0\" align=\"absmiddle\"></a>&nbsp;";  
  echo " <a href=$itself><img src=\"./mon-icons/icon_start.php\" vspace=\"5\" hspace=\"1\" border=\"0\" align=\"absmiddle\"></a>\n</div>";  

} elseif ( $ifsel ) {
  
  // If selection of search object and nr. of attributres is made, display options:

  $scope = $_POST;
  
  $object  = $scope["object"];
  $nlines  = $scope["nlines"];
  if ( !$nlines ) $nlines = 6;

  echo "<h3>".$errors["416"].$object."</h3>\n";
  echo "<div><i>".$errors["417"]."</i></div>\n";
  echo "<div><i>".$errors["418"]."</i></div><br><br>\n";
  $attwin = popup($itself,650,300,7,$lang,$debug);
  
  echo "<form name=\"attlist\" action=\"$itself\" method=\"post\" onReset=\"clean_reset()\">";  
  echo "<table border=\"0\">\n";

  $rcol = "#ccffff";
  for ( $i = 0; $i < $nlines; $i++ ) {
    echo "<tr bgcolor=\"$rcol\"><td><select name=attributes[]>\n";
    switch ( $rcol ) {
    case "#ccffff":
      $rcol = "#ccffcc";
      break;
    case "#ccffcc":
      $rcol = "#ccffff";
      break;
    }
    //    natsort($isattr);
    echo "<option value=>------".$errors["427"]."------</option>\n";
    foreach ( $isattr as $mdsatt => $name ) {
      $components = explode("-",$mdsatt);
      if ( $components[1] == $object ) echo "<option value=$mdsatt>$name</option>\n";
    }
    echo "</select></td>\n";
    echo "<td><input type=\"hidden\" name=\"scope\" value=\"$object\">";
    echo "<select name=signs[]>\n";
    echo "<option value=\"=\">=</option>\n";
    echo "<option value=\"~\">~</option>\n";
    echo "<option value=\">=\">>=</option>\n";
    echo "<option value=\"<=\"><=</option>\n";
    echo "<option value=\"!=\">!=</option>\n";
    echo "<option value=\"!~\">!~</option>\n";
    echo "</select></td>\n";
    echo "<td><input type=\"text\" size=\"20\" name=filters[]></td></tr>\n";
  }
  echo "<tr><td colspan=\"3\" align=\"right\"><br><input type=\"reset\" value=\"".$errors["428"]."\">&nbsp;&nbsp;<input type=\"submit\" value=\"".$errors["429"]."\" name=\"submit\"></td></tr>\n";
  echo "</table></form>\n";
  echo " <a href=\"$itself\"><img src=\"./mon-icons/icon_start.php\" vspace=\"1\" hspace=\"3\" border=\"0\" align=\"absmiddle\"></a>\n";
} else {

  echo "<h3>".$errors["419"]."</h3>\n";
  echo "<form name=\"scope\" action=\"$itself\" method=\"post\" onReset=\"clean_reset()\">";
  echo "<p>".$errors["423"]." <select name=\"object\">\n";
  echo "<option value=\"cluster\">".$errors["410"]."</option>\n";
  echo "<option value=\"queue\">".$errors["411"]."</option>\n";
  echo "<option value=\"job\">".$errors["412"]."</option>\n";
  echo "<option value=\"authuser\">".$errors["413"]."</option>\n";
  echo "<option value=\"se\">".$errors["414"]."</option>\n";
  echo "<option value=\"rc\">".$errors["415"]."</option>\n";
  echo "</select>\n";
  echo "&nbsp;&nbsp;<nobr>".$errors["424"]." <input type=\"text\" size=\"2\" name=\"nlines\">\n";
  echo "&nbsp;&nbsp;<input type=\"submit\" value=\"".$errors["426"]."\" name=\"select\"></nobr>\n";
  echo "</form>\n";
}

echo "</div>\n";

$toppage->close();

?>
