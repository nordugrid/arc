<?php
 
// Author: oxana.smirnova@hep.lu.se

// Draws a red rectangle with "Force refresh" inside

header ("Content-type: image/png");
$im         = @imagecreate (100, 24);
$background = imagecolorallocate ($im, 255, 255, 255);
$tcol       = imagecolorallocate ($im, 98,98,98);
$trans      = imagecolortransparent($im,$background);
ImageRectangle($im, 1, 1, 79, 23, $tcol);
imagestring ($im, 3, 5, 5,  "START OVER", $tcol);
imagepng ($im);
ImageDestroy($im);

?>