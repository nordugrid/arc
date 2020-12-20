<?php

// Author: oxana.smirnova@hep.lu.se

 /**
 * A class to set up HTML pages
 */

class LmDoc { // class LmDoc

  var $module;

  var $version;

  var $title      = "";
  var $refresh    = 120;
  var $bg         = "#ffffff";
  var $lc         = "#cc0000";

  var $language   = "en";
  var $strings    = array();
  var $errors     = array();
  var $countries  = array();
  var $mdsattr    = array();
  var $isattr     = array();
  var $loadmon    = array();
  var $clusdes    = array();
  var $jobstat    = array();
  var $volist     = array();
  var $vousers    = array();
  var $userlist   = array();
  var $userres    = array();
  var $attlist    = array();
  var $quelist    = array();
  var $sestat     = array();
  var $allusers   = array();
  var $ldapdump   = array();

  var $giislist   = array();
  var $emirslist  = array();
  var $cert;

  var $archery_list = array();

  /**
   * @return LmDoc
   * @param wintyp string
   * @param extratitle string
   * @desc Creates a new HTML document of type wintyp, title appended with an optional extratitle
   */
  function __construct ( $wintyp="", $extratitle="" ) { // function LmDoc ()
      header("Content-type: text/html; charset=utf-8");
      ob_implicit_flush(0);
      ob_start();
      //      ini_set("max_execution_time","120");
      //      ini_set("memory_limit","15M");

      //      set_time_limit(10);

      // Dumps the page start HTML code 
      //echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";
      //echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n";
      echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD W3 HTML//EN\">\n";
      echo "<html><head>\n";
      echo "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n";
      echo "<meta name=\"robots\" content=\"noindex,nofollow\">\n";
      echo "<script type=\"text/javascript\" src=\"monitor.js\"></script>\n";
      echo "<script type=\"text/javascript\" src=\"mylo.js\"></script>\n";

      if ( $wintyp ) {

	    $this->module = $wintyp;

        // Localize

        $yaccept = @$_SERVER["HTTP_ACCEPT_LANGUAGE"] ;
        if ( !$yaccept ) $yaccept = "en";
        if ( FORCE_LANG != "default" ) $yaccept = FORCE_LANG;
        $yazyk  = "en";
        $yazyki = explode(",",$yaccept);
        foreach ( $yazyki as $option ) {
              if ( $yazyk != "en" ) continue;
          $option = trim($option);
              $option = substr($option,0,2);
          // some sniffing
          // touch("test/".$option);
          // echo "<!--$option-->\n";
          $locfile = $option.".inc";
          if ( !file_exists("lang/".$locfile) ) continue;
          $yazyk   = $option;
            }
        $locfile = $yazyk.".inc";

        include $locfile;

        setlocale(LC_ALL, $yazyk);

        $this->language  = $yazyk;
        $this->strings   = $message;
        $this->errors    = $message["errors"];
        $this->countries = $message["tlconvert"];
        $this->mdsattr   = $message["mdsattr"];
        $this->isattr    = $message["isattr"];

        // Assigns $this->clusdes   = $message["clusdes"];
        $this->$wintyp   = $message[$wintyp];

        $toptitle = $message[$wintyp][0];

        // Set page parameters
        require ('settings.inc');

        $inpnam   = implode("_",array("def",$wintyp));

        // Page style definitions (see settings.inc)

        // Sets top window title
        $this->title    = ( $toptitle ) ? $toptitle : "";

        // Refresh rate
        $this->refresh  = (${$inpnam}["refresh"])  ? ${$inpnam}["refresh"]  : 0;

        // Background and link colors
        $this->bg       = (${$inpnam}["bgcolor"])  ? ${$inpnam}["bgcolor"]  : "#ffffff";
        $this->lc       = (${$inpnam}["lcolor"])   ? ${$inpnam}["lcolor"]   : "#cc0000";

        // Dumps the header HTML code
        $titles = explode(":",$this->title); // sometimes titles are many
        echo "<title>".$titles[0]." ".$extratitle."</title>\n";
        if ( $this->refresh ) echo "<meta http-equiv=\"refresh\" content=".$this->refresh.">\n";
        echo "<style>\n";
        echo "<!--\n";
        echo "A {text-decoration:none; }\n";
        echo "A:link {color: ".$this->lc."; }\n";
        echo "A:visited {color: ".$this->lc."; }\n";
        echo "A:first-child {color: ".$this->lc."; }\n";
        echo "BODY {background-color: ".$this->bg."; font-size:small; height:auto;}\n";
        echo "P {padding: 0em; margin: 10px 0px 6px 0px; font-size:small; }\n";
        echo "TD {font-size:small;}\n";
        echo "DT {text-align: justify;}\n";
        echo "DD {text-align: justify;}\n";
        echo ".right {text-align: right; padding: 0em; margin: 0em;}\n";
        echo ".fix {font-family: monospace;}\n";
        echo "//-->\n";
        echo "</style>\n";

        // define giislist
        if ( ! isset($emirslist) ) {
          $emirslist = array ();
        }
        if ( ! isset($archery_list)) {
          $archery_list = array ();
        }
        $this->giislist  = $giislist;
        $this->emirslist = $emirslist;
        $this->cert = $cert;
        $this->archery_list = $archery_list;
      }
      
      // Finishes HTML header, starts document body
      echo "</head>\n";
      echo "<body>\n";
      echo "<center>\n";
    }

  /**
   * @return void
   * @param errors array
   * @param title string
   * @param subtitle string
   * @desc Makes an opening Monitor header
   */
  function tabletop ( $toptitle="", $subtitle="" ) { // function tabletop()

    $lang = FORCE_LANG;

    echo "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n";
    echo "<tr><td nowrap align=\"center\" colspan=\"2\"><b>".$toptitle."</b></td>\n";
    echo "<tr><td nowrap align=\"left\"><b><i>".$subtitle."</i></b></td>\n";
    echo "<td nowrap align=\"right\"><a href=\"javascript: window.location.reload()\">\n";
    echo "     <img src=\"./mon-icons/icon-refresh.png\" vspace=\"4\" width=\"24\" height=\"24\" border=\"0\" align=\"absmiddle\" title=\"".$this->errors["301"]."\" alt=\"".$this->errors["301"]."\"></a>\n";
    echo "     <a href=\"javascript:(self.print();)\" onClick=\"self.print();\">\n";
    echo "     <img src=\"./mon-icons/icon-print.png\" vspace=\"4\" width=\"24\" height=\"24\" border=\"0\" align=\"absmiddle\" title=\"".$this->errors["302"]."\" alt=\"".$this->errors["302"]."\"></a>\n";
    echo "     <a href=\"javascript:monitor('help.php?module=".$this->module."&lang=".$lang."',400,300,10);\" onClick=\"javascript:monitor('help.php?module=".$this->module."',400,300,10);\">\n";
    echo "     <img src=\"./mon-icons/icon-help.png\" vspace=\"4\" width=\"24\" height=\"24\" border=\"0\" align=\"absmiddle\" title=\"".$this->errors["303"]."\" alt=\"".$this->errors["303"]."\"></a>\n";
    echo "     <a href=\"javascript:(self.close();)\" onClick=\"self.close();\">\n";
    echo "     <img src=\"./mon-icons/icon-close.png\" vspace=\"4\" width=\"24\" height=\"24\" border=\"0\" align=\"absmiddle\" title=\"".$this->errors["304"]."\" alt=\"".$this->errors["304"]."\"></a>\n";
    echo "</td></tr></table>\n";
  }
	
  /**
   * @return string
   * @desc returns version number from README
   */
  function getVersion ()
  {

    $v = "N/A";

    if ( file_exists("README") ) {
      $readme = fopen("README","r");
      $fline  = fgets($readme);
      $v      = substr(stristr($fline,"version "),8);
      fclose($readme);
    }

    $this->version = $v;

    return $v;

  }

  /**
   * @return void
   * @desc Closes an HTML document
   */
  function close ()
    {
      // Closes the HTML document
      echo "\n</center>\n";
      echo "</body></html>\n";
      while (ob_get_level() > 0) {
        ob_end_flush();
      }
      ob_implicit_flush();
    }
	
}

/** 
 * Below are some generic functions, non-class-specific
 *
 * function dbgmsg ( string ) : prints out a message and flushes output; useful for debugging
 * function popup ( string, int, int, int ) : opens up a new window, depending on the client
 */

/**
 * @return void
 * @param dbgtxt string
 * @desc Outputs a debug message outside the table
 */
function dbgmsg( $dbgtxt="Debug" )
{
  echo "$dbgtxt\n";
  flush();
}

/**
 * @return void
 * @param contact string
 * @param x int
 * @param y int
 * @param n int
 * @param lang string
 * @param debug int
 * @desc Returns a new monitor window URL
 */
$agent = @$_SERVER["HTTP_USER_AGENT"] ;
if ( !defined("USERAGENT") ) define("USERAGENT",$agent);

function popup()
{
  $numargs = func_num_args();
  $contact  = func_get_arg(0);
  if ( $numargs < 2 ) {
    $x = 400;
    $y = 300;
    $n = 1;
  } elseif ( $numargs < 5 ) {
    $lang = "default";
    $debug = 0;
  } else {
    $x        = func_get_arg(1); // 400
    $y        = func_get_arg(2); // 300
    $n        = func_get_arg(3); // 1
    $lang     = func_get_arg(4);
    $debug    = func_get_arg(5);
  }
  ( USERAGENT ) ? $agent = USERAGENT : $agent = "lynx";
  if ( preg_match("/opera/i",$agent) 
       || preg_match("/lynx/i",$agent) 
       || preg_match("/konqueror/i",$agent) 
       ) return $contact;
  //  $link  = "javascript:monitor('".$contact."',$x,$y,$n)";
  if ( $lang != "default" && $lang != FALSE ) {
    if ( strpos($contact,"?") ) {
      $contact .= "&" ;
        } else {
      $contact .= "?" ;
    }
    $contact .= "lang=$lang";
  }
  if ( $debug ) {
    if ( strpos($contact,"?") ) {
      $contact .= "&" ;
        } else {
      $contact .= "?" ;
    }
    $contact .= "debug=$debug";
  }
  $link = $contact."\" target=\"win".$n."\" onClick=\"monitor('".$contact."',$x,$y,$n); return false";
  return $link;
}

?>
