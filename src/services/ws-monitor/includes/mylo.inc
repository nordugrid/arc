<?php
function mylo ( $email )
{

  $mailcomp = array();

  // do some e-mail masquerading
  $m1 = ""; $m2 = ""; $m3 = ""; $m4 = "";
  $dot1   = FALSE;
  $dot2   = FALSE;
  $group1 = array ();

  $group1 = explode("@",$email);
  if ( count($group1) == 2 ) {
    $dot1   = strrpos($group1[0],".");
    $dot2   = strrpos($group1[1],".");
    if ( $dot2 !== FALSE ) {
      if ( $dot1 === FALSE ) {
	$m1 = $group1[0];
      } else {
	$m1 = substr($group1[0],0,$dot1);
	$m2 = substr($group1[0],$dot1+1);
      }
      $m3 = substr($group1[1],0,$dot2);
      $m4 = substr($group1[1],$dot2+1);
      
      $mailcomp = array($m1,$m2,$m3,$m4);
    }
  }
  return $mailcomp;

}
?>