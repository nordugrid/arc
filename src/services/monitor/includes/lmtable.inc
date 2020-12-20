<?php
 
// Author: oxana.smirnova@hep.lu.se

/*
 * Describes the following classes for the NorduGrid Load Monitor tables:
 *  LmTable     - constructs all other tables characterized by:
 *                header row color
 *                body color
 *                title font
 *                main text font
 *                array of column headers
 *                array of column sizes
 *  LmTableSp   - extends LmTable by adding horizontal grid
 *  LmTableFree - extends LmTableSp by using arbitrary number of columns
 *
 *  -- LmTable methods:
 *       LmTable($type)                 - initializes $type table headers
 *       addrow($array,$bgcolor)        - adds a contents row 
 *       addspacer($bgcolor)            - adds a spacer line (horizontal grid)
 *       adderror($text,$bgcolor)       - adds an error message across the table
 *       rowspan($nrows,$text,$bgcolor) - adds a cell spanning $nrows rows
 *       close()                        - closes the table
 *
 * Author: O.Smirnova (July 2002)
 */ 

/*
 * CLASS LmTable
 */

class LmTable {
  
  var $color_header;
  var $color_bg;
  
  var $font_title;
  var $font_main;
  
  var $columns;
  var $contents;
  var $ncols;
  
  var $errtxt;
  var $nrows;
  
  /**
   * @return LmTable
   * @param wintyp string
   * @param locset array
   * @desc Starts an HTML table
   */
  function __construct( $wintyp, $locset, $schema = "NG") {
    /*
     * Initialisation of a table: 
     * definition of colors
     * 
     * Input: 
     *   string $wintyp - table style
     *   array $locset  - set of localized headers etc
     */
    
    ob_implicit_flush(TRUE);
#    ob_start();
    while (ob_get_level() > 0) {
      ob_end_flush();
    }


    require ('settings.inc');

    $inpnam = implode("_",array("def",$wintyp));
    //    $xeader = implode("_",array("header",$wintyp));
    
    $this->color_header = (${$inpnam}["thcolor"]) ? ${$inpnam}["thcolor"] : "#999999";
    $this->color_bg     = (${$inpnam}["tbcolor"]) ? ${$inpnam}["tbcolor"] : "#f0f0f0";
    $this->font_title   = (${$inpnam}["thfont"])  ? ${$inpnam}["thfont"]  : "color=\"#ffffff\"";
    $this->font_main    = (${$inpnam}["tbfont"])  ? ${$inpnam}["tbfont"]  : "color=\"#000000\"";
    $this->columns      = $locset;
    $this->ncols        = 0;
    
    echo "<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%\" bgcolor=\"".$this->color_bg."\">\n";
    echo "<tr bgcolor=\"".$this->color_header."\">\n";
    $colnr = 0;
    if ( $wintyp == "clusdes" && $schema != "GLUE2" ) {
      $position = 3;
      $keys = array_keys($locset);
      unset($locset[$keys[$position]]);
    }
    foreach ( $locset as $colnam => $colwid) {
      if ( $colnam == "0" || $colnam == "help" ) continue;
      $this->ncols ++;
      $colnr++;
      $value = $colnam;
      if (  $schema == "GLUE2" && $value == "Queue") {
        $value = "Share Name";
      }
      // Specific sorting links for the front module
      if ( $wintyp == "loadmon" ) {
	// Keep old arguments, if any, except of order
	$allargs = "";
	foreach ( $_GET as $argm => $argval ) {
	  if ( $argm == "order" ) continue;
	  $allargs .= $argm."=".$argval."&";
	}
	$str1 = "<a href=\"".$_SERVER['PHP_SELF']."?".$allargs."order=";
	$str2 = "\"><font ".$this->font_title.">".$value."</font></a>";
	if ( $colnr == 1 ) $value = $str1."country".$str2;
	elseif ( $colnr == 3 ) $value = $str1."cpu".$str2;
	elseif ( $colnr == 4 ) $value = $str1."grun".$str2;
      }
      $width = ($colwid)?$colwid:"1%";
      echo "<td align=\"left\" width=\"$width\"><nobr><font ".$this->font_title.">&nbsp;$value&nbsp;</font></nobr></td>\n";
    }
    echo "</tr>\n";
  }
  
  /**
   * @return void
   * @param contents array 
   * @desc Draws a table row
   */
  function addrow( $contents, $bgcol="" ) {
    
    if ( count($contents) != $this->ncols ) {
      $this->adderror("Incompatible data");
      return 1;
    }
    
    $this->contents = $contents;

    if ($bgcol) {
      echo "<tr bgcolor=\"$bgcol\">\n";
    } else {
      echo "<tr>\n";
    }
    foreach ($contents as $colent) {
      $value = $colent;
      echo "<td align=\"left\" valign=\"middle\"><nobr><font ".$this->font_main.">&nbsp;$value&nbsp;</font></nobr></td>\n";
    }
    echo "</tr>\n";
    
  }
  
  /**
   * @return void
   * @param color string
   * @desc Draws a spanning row containing a spacer
   */
  function addspacer( $color="#000000" ) {
    
    echo "<tr>\n";
    echo "<td colspan=\"".$this->ncols."\" bgcolor=\"$color\" height=\"0\"><img src=\"./mon-icons/icon_spacer.php\" width=\"1\" height=\"1\" border=\"0\" alt=\"\"></td>";
    echo "</tr>\n";
    
  }
   
  /**
   * @return void
   * @param errtxt string 
   * @desc Draws a spanning row containing error message
   */
  function adderror( $errtxt="Error", $bgcol="" ) {
    
    $this->errtxt  = $errtxt;

    echo "<tr>\n";
    echo "<td nowrap align=\"left\" colspan=\"".$this->ncols."\"";
    if ($bgcol) echo " bgcolor=\"$bgcol\"";
    echo "><font ".$this->font_main."><font color=\"#000000\">&nbsp;$errtxt</font></font></td>";
    echo "</tr>\n";
    
  }
   
  /**
   * @return void
   * @param errtxt string 
   * @param nrows integer
   * @param color string
   * @desc Adds a cell spanning $nrows rows
   */
  function rowspan( $nrows, $errtxt="&nbsp;", $color="#ffffcc" ) {
    
    $this->errtxt = $errtxt;

    $ncols  = $this->ncols - 1;
    $nrows  = $nrows + 1;

    echo "<tr>\n";
    echo "<td nowrap align=\"left\" valign=\"middle\" rowspan=\"$nrows\" bgcolor=\"$color\">&nbsp;$errtxt</td>";
    echo "<td nowrap colspan=\"$ncols\" bgcolor=\"$color\" height=\"0\"><img src=\"./mon-icons/icon_spacer.php\" width=\"1\" height=\"1\" border=\"0\" alt=\"\"></td>";
    echo "</tr>\n";
    
  }
   
  /**
   * @return void
   * @desc Closes a table
   */
  function close() {
    echo "</table>\n";
    
#    ob_end_flush();
    ob_implicit_flush(FALSE);
  }
  
}

class LmTableSp extends LmTable {

  var $spcolor;
  
  /**
   * @return void
   * @param contents array 
   * @param color string 
   * @desc Draws a table row with a spacer above
   */
  function addrow( $contents, $bgcol="", $color="#ffffff" ) {
    
    $ncols = count($contents);
    
    $this->contents = $contents;

    if ($bgcol) {
      echo "<tr nowrap bgcolor=\"$bgcol\">\n";
    } else {
      echo "<tr nowrap>\n";
    }
    foreach ($contents as $colent) {
       $value = $colent;
      echo "<td align=\"left\" valign=\"middle\"><nobr><font ".$this->font_main.">&nbsp;$value&nbsp;</font></nobr></td>\n";
    }
    echo "</tr>\n";

    echo "<tr>\n";

    echo "<td colspan=\"$ncols\" bgcolor=\"$color\" height=\"1\"><img src=\"./mon-icons/icon_spacer.php\" width=\"1\" height=\"1\" alt=\"\"></td>";
    echo "</tr>\n";
    
  }
  
    /**
     * @return void
     * @param errtxt string
 	   * @param color string 
     * @desc Draws a spanning row containing error message
     */
    function adderror( $errtxt="Error", $color="#ffffff", $bgcol="" ) {
    
    $this->errtxt  = $errtxt;
    
    $ncols  = $this->ncols;
    $tospan = $this->rowspan;
    if ( $tospan ) $ncols = $ncols - 1;

    echo "<tr>\n";
    echo "<td nowrap colspan=\"$ncols\" bgcolor=\"$color\" height=\"0\"><img src=\"./mon-icons/icon_spacer.php\" width=\"1\" height=\"1\" border=\"0\" alt=\"\"></td>";
    echo "</tr>\n";
  
    echo "<tr>\n";
    echo "<td nowrap align=\"left\" colspan=\"".$this->ncols."\"";
    if ($bgcol) echo " bgcolor=\"$bgcol\"";
    echo "><font ".$this->font_main."><font color=\"#990000\">&nbsp;$errtxt</font></font></td>";
    echo "</tr>\n";
    
  }

  /**
   * @return void
   * @param errtxt string 
   * @param nrows integer
   * @param color string 
   * @desc Adds a cell spanning $nrows rows
   */
  function rowspan( $nrows, $errtxt="&nbsp;", $color="#ffffcc" ) {
    
    $this->errtxt = $errtxt;

    $ncols  = $this->ncols - 1;
    $nrows  = (2 * $nrows) + 1;

    echo "<tr>\n";
    echo "<td nowrap align=\"left\" valign=\"middle\" rowspan=\"$nrows\" bgcolor=\"$color\">&nbsp;$errtxt</td>";
    echo "<td nowrap colspan=\"$ncols\" bgcolor=\"$color\" height=\"0\"><img src=\"./mon-icons/icon_spacer.php\" width=\"1\" height=\"1\" border=\"0\" alt=\"\"></td>";
    echo "</tr>\n";
    
  }
   


}

class LmTableFree extends LmTableSp {

  /**
   * @return LmTableFree
   * @param headers array
   * @desc Starts an HTML table
   */
  function __construct( $headers ) {
    
    ob_implicit_flush(0);
    ob_start();

    $this->color_header = "#666666";
    $this->color_bg     = "#f0f0f0";
    $this->font_title   = "color=\"#ffffff\"";
    $this->font_main    = "color=\"#000000\"";
    $this->columns      = count($headers);
    $this->ncols        = 0;
    
    echo "<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%\" bgcolor=\"".$this->color_bg."\">\n";
    echo "<tr bgcolor=\"".$this->color_header."\">\n";
    foreach ( $headers as $colnam ) {
      $this->ncols ++;
      $value = $colnam;
      $width = "1%";
      echo "<td align=\"left\" width=\"$width\"><nobr><font ".$this->font_title.">&nbsp;$value&nbsp;</font></nobr></td>\n";
    }
    echo "</tr>\n";
  }
  
}

?>
