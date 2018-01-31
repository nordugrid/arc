<?php
/**
 * @return integer
 * @param string $fname
 * @desc Returns modification time of a file, or 100000 if no file found
 */
function last_modified($fname)
{
  $tdif = 100000;
  $now  = time();

  if ( file_exists($fname) ) {
    $last_touched = filemtime($fname);
    $tdif = $now - $last_touched;
  }
  return $tdif;
}

/**
 * @return void
 * @param string $fname
 * @param array $trows
 * @desc Dumps a table to a file
 */
function cache_table($fname,$trows)
{
  $fhandle = @fopen($fname,"wb");
  $arstring = "array(";
  $i = 0;
  $imax = count($trows);
  foreach ($trows as $row) {
    $i++;
    $arstring .= "array(";
    $j = 0;
    $jmax = count($row);
    foreach ($row as $value) {
      $j++;
      $value = addcslashes($value,"\"");
      $arstring .= "\"$value\"";
      if ( $j < $jmax ) $arstring .= ",";
    }
    $arstring .= ")";
    if ( $i < $imax ) $arstring .= ",";
  }
  $arstring .= ")";
  @fwrite($fhandle,$arstring,strlen($arstring));
  @fclose($fhandle);
}

/**
 * @return array
 * @param string $fname
 * @param integer $ttl
 * @desc Checks whether query is cached already and returns the result or FALSE
 */
function get_from_cache($fname,$ttl)
{
  $cache = FALSE;

  if ( last_modified($fname) < $ttl ) {
    $contents = file_get_contents($fname);
    eval("\$cache = $contents;");
    if ( count($cache) < 1 ) $cache = FALSE;
  }
  return $cache;
}

?>