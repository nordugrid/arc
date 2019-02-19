<?php

// Author: manf@grid.org.ua

if ( file_exists("blacklist.inc") ) include('blacklist.inc');


/**
 * @param $error_str
 */
function dbgerr_html($error_str){
    dbgmsg("<div><span style='color: #ff0000; font-weight: bold'>" .$error_str."</span></div>\n");
}

/**
 * @param $hostname
 * @return int
 */
function check_blacklist($hostname) {
    global $blacklist;
    if ( ! isset($blacklist) ) {
        return 0;
    }
    if ( in_array($hostname, $blacklist) ) {
        return $blacklist[$hostname];
    }
    return 0;
}


/**
 * @param $endpoint
 * @param $errors
 * @param $debug
 * @return array
 */
function query_dns_archery($endpoint, $errors, $debug)
{
    $archery_endpoints = array();
    // get dns record hostname according to ARCHERY entree point convention
    if (substr($endpoint, 0, 6) == 'dns://') {
        $dns_endpoint = substr($endpoint, 6);
    } else {
        $dns_endpoint = '_archery.' . $endpoint;
    }
    // perform query
    $dnsquery = dns_get_record($dns_endpoint, DNS_TXT);
    if ( $dnsquery === FALSE ) {
        if ( $debug ) dbgerr_html($errors["132"].$endpoint);
        return $archery_endpoints;
    }
    // parse query
    foreach ( $dnsquery as $dnsrr ) {
        if (isset($dnsrr['entries'])) {
            foreach ($dnsrr['entries'] as $dnsrr_value) {
                $erecord_arr = array();
                $akv = explode(' ', $dnsrr_value);
                foreach ($akv as $kv) {
                    $ae = explode('=', $kv, 2);
                    if ( count($ae) == 2 ) {
                        $erecord_arr[$ae[0]] = $ae[1];
                    }
                }
                $archery_endpoints[] = $erecord_arr;
            }
        } else {
            if ( $debug ) dbgerr_html($errors["132"].$endpoint);
        }
    }

    return $archery_endpoints;
}


/**
 * @param $endpoint
 * @param $schema
 * @param $errors
 * @param int $debug
 * @param int $looplimit
 * @return array
 */
function recursive_archery_info ($endpoint, $schema, $errors, $debug=0, $looplimit=5) {
    $endpoints = array();
    // Just in case recursion limit
    if ( $looplimit == 0 ) {
        dbgerr_html($errors["133"].$endpoint);
        return $endpoints;
    }
    // Query archery for endpoints info
    $archery_endpoints = query_dns_archery($endpoint, $errors, $debug);
    foreach ($archery_endpoints as $ainfo) {
        if ($ainfo['t'] == 'org.nordugrid.archery' OR $ainfo['t'] == 'archery' OR 
            $ainfo['t'] == 'archery.service' OR $ainfo['t'] == 'archery.group' ) {
            if (isset($ainfo['s'])) {
                if ($ainfo['s'] != "1") {
                    continue;
                }
            }
            $more_endpoints = recursive_archery_info ($ainfo['u'], $schema, $errors, $debug=0, $looplimit-1);
            $endpoints = array_merge($endpoints, $more_endpoints);
        } elseif ($ainfo['t'] == 'org.nordugrid.ldapegiis') {
            //TODO: invoke egiis query
            continue;
        } elseif ($ainfo['t'] == 'org.nordugrid.emir') {
            //TODO: invoke emir query (or maybe we should abandone it)
            continue;
        } elseif ($ainfo['t'] == 'org.nordugrid.ldapng') {
            if ( $schema !== 'NG' ) continue;
            // ldap://<host>:2135/Mds-Vo-Name=local,o=grid
            $parsed_url = array();
            if ( preg_match('/^ldap:\/\/(?P<host>[^:]+):(?<port>[0-9]+)\/(?P<base>.*)/', $ainfo['u'], $parsed_url) ) {
                if ( check_blacklist($parsed_url['host'])) {
                    if ( $debug ) dbgerr_html($errors["122"].$parsed_url['host']);
                    continue;
                }
                $endpoints[] = array (
                    'host' => $parsed_url['host'],
                    'port' => $parsed_url['port'],
                    'base' => "nordugrid-cluster-name=".$parsed_url['host'].",".$parsed_url['base']
                );
            }
        } elseif ($ainfo['t'] == 'org.nordugrid.ldapglue2') {
            if ( $schema !== 'GLUE2' ) continue;
            // ldap://<host>:2135/o=glue
            $parsed_url = array();
            if ( preg_match('/^ldap:\/\/(?P<host>[^:]+):(?<port>[0-9]+)\/(?P<base>.*)/', $ainfo['u'], $parsed_url) ) {
                if ( check_blacklist($parsed_url['host'])) {
                    if ( $debug ) dbgerr_html($errors["122"].$parsed_url['host']);
                    continue;
                }
                $endpoints[] = array (
                    'host' => $parsed_url['host'],
                    'port' => $parsed_url['port'],
                    // dirty hack, monitor only works with array of ldapng endpoints even for GLUE2 :-)
                    'base' => "nordugrid-cluster-name=".$parsed_url['host'].",".DN_LOCAL
                );
            }
        } else {
            // skip all unsupported endpoints (e.g. submission endpoints, WS endpoints, etc)
            continue;
        }
    }
    return $endpoints;
}

/**
 * @return array
 * @param archery_list array
 * @param schema string
 * @param debug integer
 * @param loopcnt integer
 * @desc Returns list of LDAP endpoints
 */
function archery_info($archery_list, $schema, $errors, $debug="0") {
    // show the debug message regarding ARCHERY timeouts
    if($debug && ! empty($archery_list)) {
        dbgmsg("<div align=\"left\"><i>:::&gt; " . $errors["131"] . " &lt;:::</div><br/>");
    }

    // start recursively querying ARCHERY
    $entries = array();
    foreach ( $archery_list as $archery ) {
        $entries = array_merge($entries, recursive_archery_info($archery['endpoint'], $schema, $errors, $debug));
    }

    return $entries;
}

?>
