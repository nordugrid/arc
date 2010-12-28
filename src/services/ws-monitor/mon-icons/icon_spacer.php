<?php

// Author: oxana.smirnova@hep.lu.se

// Draws a 1x1 px spacer (PNG)

header ("Content-type: image/png");
$im      = @imagecreate(1,1);
$bgcolor = imagecolorallocate($im,255,255,255);
imagepng ($im);
ImageDestroy($im);

?>