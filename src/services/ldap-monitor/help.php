<?php

// Author: oxana.smirnova@quark.lu.se

/**
 * @desc Help window
 */

$module = $_GET["module"];

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');

$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);

$toppage = new LmDoc($module);

$data = &$toppage->$module;

$helptext = $data["help"];

echo $helptext;

// Done

$toppage->close();

?>
