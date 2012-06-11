<?php

// Author: gabor.szigeti@niif.hu

/**
 * @return array
 * @param emirslist array
 * @param element string
 * @param debug integer
 * @param loopcnt integer
 * @desc Returns base EMIR info
 */
function emirs_info($emirslist,$element,$errors,$gentries,$debug="0",$cert)
{
  /*
   * Array $emirslist is assumed to be a 4-dim array of the type:
   * array(
   *       array("schema" => "https",
   *             "host"   => "testbed-emi5.grid.upjs.sk,
   *             "port"   => "54321",
   *             "base"   => "mds-vo-name=NorduGrid,o=grid"),...)
   */

  $tlim = 2;
  $tout = 5;
  if($debug && count($emirslist) < 5) dbgmsg("<div align=\"left\"><i>:::&gt; ".$errors["130"].$tlim.$errors["102"].$tout.$errors["103"]." &lt;:::</div><BR>");


  $nemirs    = count($emirslist);

  $counter   = count($gentries);
  
  $tag       = array();
  $entries   = $gentries;
  $blacklist = array();

  if ( file_exists("blacklist.inc") ) include('blacklist.inc');

  // Loop on entered EMIR sites
  // If a host is blacklisted, skip
  // If a host is a cluster (GRIS), keep and skip
  // If a host is any other local GRIS, skip

  for ( $ig = 0; $ig < $nemirs; $ig++ ) {
    $eschema = $emirslist[$ig]["schema"];
    $ehost = $emirslist[$ig]["host"];
    if ( @$blacklist[$ehost] ) {
      if ( $debug ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>".$errors["122"]."$ehost</b></font></div>\n");
      continue;
    }
    $eport = $emirslist[$ig]["port"];
    $ebase = $emirslist[$ig]["base"];
    if ( preg_match("/$element/i",$ebase) ) {
      // Invent a "fake DN" for host tagging and skip duplicated entries
      $fakedn  = "hn=".$ehost.", ".$ebase;
      if ( @$tag[$fakedn] ) continue;
      $tag[$fakedn] = 1;
      continue;
    } elseif ( preg_match("/local/i",$ebase) ) {
      if ( $debug ) dbgmsg("<div align=\"left\"><b>".$errors["115"].$ehost." (".$errors["116"].$element.")</b>");
      continue;
    }
    if ( $debug == 2 ) dbgmsg("<div align=\"left\">".$errors["117"]."<b>$ehost</b>...");

    // Connection to EMIR
    $path = "services";
    $query = "Service_Endpoint_Capability=information.discovery.resource&Service_Endpoint_Technology=ldap";

    $res = http_request('GET',
                        $eschema."://".$ehost.":".$eport."/".$path."?".$query,
                        $data,
                        $cert,
                        $referer='');

    if ($res["status"] == "ok"){
      if ( $debug == 2 ) dbgmsg($errors["118"]);

      $json_a=json_decode($res["content"], true);
      $nrecords = count($json_a);

      for ($i = 0; $i < $nrecords; $i++) {
        $url = parse_url($json_a[$i]["Service_Endpoint_URL"]);
        $curhost = $url["host"];
        /*
         * URL structure:
         *    * ldapng:    ldap://<url>:2135/Mds-Vo-Name=local,o=grid
         *    * ldapglue1: ldap://<url>:2135/Mds-Vo-Name=resource,o=grid
         *    * ldapglue2: ldap://<url>:2135/o=glue
         */
        // Introduce "fake" DN for tagging purpose - helps skipping sites registering twice
        $fakedn  = "hn=".$url["host"].", ".$url["base"];
        if ( @$tag[$fakedn] ) continue;

        if ( @$blacklist[$curhost] ) {
          if ( $debug>0 ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>".$errors["122"]."$curhost</b></font></div>\n");
          continue;
        }
        //TODO: check validite
        //if ( $curstat != "VALID" ) {
        //  if ( $debug ) dbgmsg("<div align=\"left\"><font color=\"red\"><b>$curstat".$errors["121"]."$fakedn</b></font></div>\n");
        //  continue;
        //}

        $entries[$counter]["host"] = $url["host"];
        $entries[$counter]["port"] = $url["port"];
        $entries[$counter]["base"] = $url["path"];
        if ( $debug == 2 ) dbgmsg("<div align=\"left\">".$errors["123"]."$base: <i>$fakedn</i></div>\n");
        $tag[$fakedn] = 1;
        $counter++;
      }

    }

  }
  if ( $debug == 2 ) dbgmsg("</div>");

  // Some debugging printout
  if ( $debug == 2 ) {
    dbgmsg("<div align=\"center\"><br><u>".$errors["119"].$element.": ".$counter."</u><br></div>");
    foreach ( $entries as $num=>$val ) dbgmsg($val["host"].":".$val["base"]."<br>");
  }

  return $entries;
}

/*
 * Send http request to the given URL of the server.
 */
function http_request($type, $url, $data, $cert, $referer='') {
 
    // Convert the data array into URL Parameters like a=b&foo=bar etc.
    $data = http_build_query($data);
 
    // parse the given URL
    $url = parse_url($url);

    if ($url['scheme'] != 'https' && $url['scheme'] != 'http') { 
        die('Error: Only HTTP(S) request are supported !');
    }
 
    // extract host and path:
    $host = $url['host'];
    $port = $url['port'];
    $path = $url['path'];
    $query= $url['query'];

    // open a socket connection on the given port - timeout: 30 sec
    $fp = stream_socket_client($host.":".$port, $errno, $errstr, 30);

    if ($url['scheme'] == 'https') {
      // add secure properties
      $context = stream_context_create();
      $result  = stream_context_set_option($context, 'ssl', 'local_cert', $cert);
      $result  = stream_context_set_option($context, 'ssl', 'passphrase', 'emi');  
    
      // open a secure socket connection on the given port - timeout: 30 sec
      $fp = stream_socket_client("ssl://".$host.":".$port, $errno, $errstr, 30, STREAM_CLIENT_CONNECT,$context);
    }

    if ($fp){
 
        // send the request headers:
        fputs($fp, $type." $path?$query HTTP/1.1\r\n");
        fputs($fp, "Host: $host\r\n");
 
        if ($referer != '')
            fputs($fp, "Referer: $referer\r\n");
 
        fputs($fp, "Content-type: application/x-www-form-urlencoded\r\n");
        fputs($fp, "Content-length: ". strlen($data) ."\r\n");
        fputs($fp, "Connection: close\r\n\r\n");
        fputs($fp, $data);
 
        $result = ''; 
        while(!feof($fp)) {
            // receive the results of the request
            $result .= fgets($fp, 128);
        }
    }
    else {
        return array(
            'status' => 'err', 
            'error' => "$errstr ($errno)"
        );
    }
 
    // close the socket connection:
    fclose($fp);
 
    // split the result header from the content
    $result = explode("\r\n\r\n", $result, 2);
 
    $header  = isset($result[0]) ? $result[0] : '';
    $content = isset($result[1]) ? $result[1] : '';
 
    // return as structured array:
    return array(
        'status' => 'ok',
        'header' => $header,
        'content' => $content
    );
}



?>
