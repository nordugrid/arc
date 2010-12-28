<?php
 
// Author: oxana.smirnova@hep.lu.se

header ("Content-type: image/png");
$im         = @imagecreate (39, 24);
$background = imagecolorallocate ($im, 255, 255, 255);
$tcol       = imagecolorallocate ($im, 98,98,98);
$trans      = imagecolortransparent($im,$background);
ImageRectangle($im, 1, 1, 36, 23, $tcol);
imagestring ($im, 3, 5, 5,  "BACK", $tcol);
imagepng ($im);
ImageDestroy($im);

?>