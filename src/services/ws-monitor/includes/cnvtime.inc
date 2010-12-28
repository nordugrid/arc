<?php

// Author: oxana.smirnova@hep.lu.se

/**
 * @return string
 * @param stamp string
 * @desc Converts time from the time stamp to a human-readable format
 */
function cnvtime ($stamp) 
{
  //$mytime=substr($stamp,0,4)."-".substr($stamp,4,2)."-".substr($stamp,6,2)." ".substr($stamp,8,2).":".substr($stamp,10,2).":".substr($stamp,12,2);
  $mytime=substr($stamp,0,10)." ".substr($stamp,11,8);
  return $mytime;
}
?>
