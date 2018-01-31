<?php
 
// Author: oxana.smirnova@hep.lu.se

/**
 * @return void
 * @param cr int
 * @param cg int
 * @param cb int
 * @desc Plots a led image of a given RGB color (PNG) 
 */
function do_led($cr=200, $cg=200, $cb=200)
{
   header ("Content-type: image/png");
   $im        = imagecreate(14,6);
   $colorBody = imagecolorallocate($im,$cr,$cg,$cb);
   imagepng ($im);
   ImageDestroy($im);
}

$c1 = $_GET["c1"];
$c2 = $_GET["c2"];
$c3 = $_GET["c3"];
do_led($c1,$c2,$c3);

?>