<?php

// Author: oxana.smirnova@hep.lu.se

$toreload = $_SERVER['PHP_SELF']."?".@$_SERVER['QUERY_STRING'];
if ( !@$_SERVER['QUERY_STRING'] ) {
  $toreload = $_SERVER['PHP_SELF']."\" onClick=\"window.location.reload('true')";
} else {
  $dsltest  = substr($toreload,0,2);
  if ($dsltest == "//") $toreload=substr($toreload,1);
}
if ( !defined("TORELOAD") ) define("TORELOAD",$toreload);

?>