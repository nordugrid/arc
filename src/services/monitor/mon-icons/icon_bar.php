<?php
 
// Author: oxana.smirnova@quark.lu.se

/**
 * @return void
 * @param x int
 * @param xg int
 * @param y int
 * @param text string
 * @desc Plots an occupancy bar (PNG)
 */
function do_bar ($x, $xg, $y, $text)
{
  /**
   * Input:
   *        x    - total occupancy
   *        xg   - dedicated occupancy (Grid)
   *        y    - bar width
   *        text - occupancy figure
   */
  header ("Content-type: image/png");
  $x1 = ceil($x*200);
  $x2 = 200-$x1;
  //  $x3 = $x1-8;
  $x3 = 92;
  $xg1 = ceil($xg*200);
  // if ($text > 9) {$x3 = $x1-16;}
  if (strlen($text) > 5) $x3 = 84;
  if (strlen($text) > 10) $x3 = 36;
  $im      = @imagecreate(200,$y);
  $bgcolor = imagecolorallocate($im,204,204,204);
  $red     = imagecolorallocate($im,97,144,0);
  $grey    = imagecolorallocate($im,176,176,176);
  //  $white   = imagecolorallocate($im,255,255,255);
  $white   = imagecolorallocate($im,48,48,48);
  if ( $x1 < $x3 ) $white   = imagecolorallocate($im,82,82,82);
  if ( $x1 )  imagefilledrectangle($im,0,0,$x1,$y,$grey);
  if ( $xg1 ) imagefilledrectangle($im,0,0,$xg1,$y,$red);
  imagestring ($im, 3, $x3, 0, $text, $white);
  imagepng ($im);
  ImageDestroy($im);
}

$x    = $_GET["x"];
$xg   = $_GET["xg"];
$y    = $_GET["y"];
$text = $_GET["text"];
do_bar($x,$xg,$y,$text);

?>