<?php

// Author: oxana.smirnova@hep.lu.se

/**
 * @desc Lists all the values of an attribute across the testbed
 */

set_include_path(get_include_path().":".getcwd()."/includes".":".getcwd()."/lang");

require_once('headfoot.inc');

$lang   = @$_GET["lang"];
if ( !$lang )  $lang    = "default"; // browser language
define("FORCE_LANG",$lang);
$debug   = @$_GET["debug"];
if ( !$debug )  $debug    = 0;

$toppage = new LmDoc("attlist");
define("TOPTIT",$toppage->title);
$strings  = &$toppage->strings;
$giislist = &$toppage->giislist;
$archery_list = &$toppage->archery_list;

require_once('attlist.inc');

$object     = $_GET["object"];
$attribute  = $_GET["attribute"];
$filter     = $_GET["filter"];
if ( !$filter ) $filter="";
if ( !$attribute ) $attribute="nordugrid-cluster-middleware";
if ( !$object ) $object="cluster";
$attribute  = rawurldecode($attribute);
$filter     = rawurldecode($filter);
if ( $attribute[1]==":") {
  $attribute  = unserialize($attribute);
  $filter     = unserialize($filter);
  $attributes = $attribute;
  $filters    = $filter;
  $n          = count($attributes);
  $signs      = array_fill(0,$n,"=");
} else {
  $attributes = array ($attribute);
  $signs      = array ("=");
  $filters    = array ($filter);
}

do_attlist($object,$attributes,$signs,$filters,$strings,$giislist,$archery_list);

// Done

$toppage->close();

?>
