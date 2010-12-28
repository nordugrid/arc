<?php

//  Authors: Ivan Martoni
//           Gabor Szigeti
//
//  Date: 2009
//
//  Description: Database layer for the webservise monitor.

class DB {
    protected $isis_urls = array();
    protected $place_of_cert;
    protected $entries = array();     //info from the ISIS
    protected $datas = array();       //info to the gridmonitor
    protected $type;                  //one of them: arex, storage
    protected $error;

    private $only_one_isis_checking = false;  //false : more then one ISIS info, true: first available ISIS information

    function __construct(/*...*/) {
        $args = func_get_args();
        /*
          1. parameter: place of the certificate (type: string)
          2. parameter: ISIS urls (type: array)
        */
        if( count($args)==0 ){
            echo "No certificate and ISIS url in the configuration!";
            return;
        }
        if( count($args)==1 ){
            echo "No certificate or ISIS url in the configuration!";
            return;
        }
        $this->place_of_cert = $args[0];
        for( $i=0, $n=count($args[1]); $i<$n; $i++ )
             $this->add($args[1][$i]);

        $this->type = "";
        $this->error = "";
    }

    public function add( /*string*/ $name = null ) {
        if( isset($name) )
            $this->isis_urls[$name] = $name;
    }

    function Country($SSPair) {
        $country;
        foreach($SSPair as $a) {
            if ($a->Name = "Country") {
                $country = $a->Value;
                break;
            }
        }
        return $country;
    }

    public function connect( $debug = null, $type_ = "arex" ) {
        $this->type = $type_;
        switch ($this->type) {
            case "arex":
                $isisRequest["QueryString"] = "/RegEntry/SrcAdv[Type = 'org.nordugrid.execution.arex']";
                break;
            case "storage":
                //$isisRequest["QueryString"] = "/RegEntry/SrcAdv[Type = 'org.nordugrid.storage.bartender']";
                $isisRequest["QueryString"] = "/RegEntry/SrcAdv[Type = 'org.nordugrid.storage.shepherd']";
                break;
            default:
                echo "Invalid type: ". $this->type;
                $this->error = "Invalid type add in the constructor: ". $this->type;
                return NULL;
        }

        ini_set("soap.wsdl_cache_enabled", "1");

        $all_response = "";
        foreach( $this->isis_urls as $url) {
            $options = array("location" => $url,
                             "exceptions" => true,
                            );

            if (preg_match("/^https:/", $url))
                $options["local_cert"] = $this->place_of_cert;

            $isis_SoapClient = new SoapClient("http://www.nordugrid.org/schemas/isis/2007/06/index.wsdl",
                                              $options
                                );
            try {
                $response = @$isis_SoapClient->Query($isisRequest);
            } catch (SoapFault $fault) {
                if($debug==2)
                    echo '<br>SOAP fault at '.$url.': '.
                    $fault->faultstring.' ('.$fault->faultcode . ")\n";
            } catch (Exception $e) {
                if($debug==2){
                    //echo 'Caught exception: ',  $e->getMessage(), "\n";
                    echo 'ISIS is not avaliable: ', $url, "\n";
                }
                continue;
            }

            $all_response = $all_response.$response->any;
            if ($this->only_one_isis_checking) break;
        }

        $this->entries = simplexml_load_string("<dummy>".$all_response."</dummy>");

        // Response processing/transformation
        for ($i=0; $i<sizeof($this->entries); $i++) {
            $sid = $this->entries->RegEntry[$i]->MetaSrcAdv->ServiceID;
            // add all SSPair elements into the array
            $sspairs = array();
            foreach($this->entries->RegEntry[$i]->SrcAdv->SSPair as $item) {
                $sspairs[(string)$item->Name] = (string)$item->Value;
            }

            $sspairs["EPR"] = (string)$this->entries->RegEntry[$i]->SrcAdv->EPR->Address;

            if (array_key_exists((string)$sid, $this->datas[(string)$sspairs["Country"]])) continue;

            $this->datas[(string)$sspairs["Country"]][(string)$sid] = $sspairs;
            $this->datas[(string)$sspairs["Country"]][((int)(count($this->datas[(string)$sspairs["Country"]])/2))] = (string)$sid;

        }
    }

    public function get_infos() {
        return $this->datas;
    }

    public function cluster_info( /*string*/ $url = null, $debug = null, $dump_or_query = true, $auth_type = null ) {
        if ($this->error != "") {
            echo $this->error;
           return NULL;
        }

        $ns = "http://www.w3.org/2005/08/addressing"; //Namespace of the WS.
        if ( $dump_or_query ){
            $header = new SOAPHeader( $ns, 'Action','http://docs.oasis-open.org/wsrf/rpw-2/GetResourcePropertyDocument/GetResourcePropertyDocumentRequest');
        } else {
            $header = new SOAPHeader( $ns, 'Action','http://docs.oasis-open.org/wsrf/rpw-2/QueryResourceProperties/QueryResourcePropertiesRequest');
        }

        ini_set("soap.wsdl_cache_enabled", "1");
        $options = array("location" => $url,
                         "exceptions" => true,
                        );

        if (preg_match("/^https:/", $url))
            $options["local_cert"] = $this->place_of_cert;

        $arex_SoapClient = new SoapClient("http://www.nordugrid.org/schemas/a-rex/arex.wsdl",
                            $options
                           );

        $arex_SoapClient->__setSoapHeaders($header);
        try{
            if ( $dump_or_query ){
                // Full LIDI information
                $response = @$arex_SoapClient->GetResourcePropertyDocument(array());
            } else {
                // Part of the LIDI information
                $query["QueryExpression"]["any"] = "//InfoRoot";
                $query["QueryExpression"]["Dialect"] = "http://www.w3.org/TR/1999/REC-xpath-19991116";
                $response = @$arex_SoapClient->QueryResourceProperties($query);
            }
            //print($response->any); /*martoni*/

            $arex_info = simplexml_load_string($response->any);
            return $arex_info;
        } catch (SoapFault $fault) {
            if($debug==2)
                echo '<br>SOAP fault at '.$url.': '.
                $fault->faultstring.' ('.$fault->faultcode . ")\n";
        } catch (Exception $e) {
            if($debug==2){
                //echo 'Caught exception: ',  $e->getMessage(), "\n";
                switch ($this->type) {
                    case "arex":
                        echo '<br>A-rex is not avaliable: ', $url, "\n";
                        break;
                    case "storage":
                        echo '<br>Storage is not avaliable: ', $url, "\n";
                        break;
                    default:
                        echo "Invalid type: ". $this->type;
                        return NULL;
                }
            }
            return;
        }
    }

    public function cluster_search($infos, /*string*/ $filter) {
        //TODO: search method is not working now
        return $infos;
    }

    public function count_entries($infos) {
        //TODO: count method is not working now
        return count($infos);
    }

    public function get_entries($infos) {
        //TODO: get method is not working now
        return $infos;
    }

    private $result = array();

    private function recursive_xml_dump($root) {
        foreach($root->children() as $children) {
            if (count($children->children()) == 0) {
                $this->result[$children->getName()][] = (string) $children;
            }
            else {
                if ($children->getName() != "ComputingActivities" &&
                    $children->getName() != "ComputingShares" &&
                    $children->getName() != "Contact")
                $this->recursive_xml_dump($children);
            }
        }
    }

    public function xml_nice_dump($xml = NULL, $scope = "cluster", $debug = 0) {
        global $errors, $strings;
        if (is_object($xml)) {
            require_once('mylo.inc');
            $this->recursive_xml_dump($xml);
            $dtable = new LmTableSp("ldapdump",$strings["ldapdump"]);
            $drowcont = array();
            if ($debug >= 10) {
                echo "Debug level: $debug<br />\n";
                echo "<!-- Pure data for debug\n";
                print_r($this->result);
                echo "\n-->\n";
            }
            $expected_names = array();
            //TODO: define visible elements
            $expected_names["cluster"] = array("URL", "CPUModel", "CPUClockSpeed", "CPUTimeScalingFactor",
                                               "CPUVersion", "CPUVendor", "PhysicalCPUs", "TotalLogicalCPUs",
                                               "TotalSlots",
                                               "Name", /*"ID",*/
                                               "OSFamily", "OSName", "OSVersion", "Platform", "VirtualMachine",
                                               "ProductName", "ProductVersion",
                                               "TrustedCA",
                                               "AppName",  "ApplicationEnvironment",
                                               "MainMemorySize", "VirtualMemorySize", "NetworkInfo",
                                               "Country", "Place", "PostCode", "Address", "Latitude", "Longitude",
                                               "OtherInfo", "Benchmark" , "BaseType",
                                               "Homogeneous", "SlotsUsedByGridJobs", "SlotsUsedByLocalJobs",
                                               "WorkingAreaFree", "WorkingAreaGuaranteed", "WorkingAreaLifeTime",
                                               "WorkingAreaShared", "WorkingAreaTotal",
                                               "ConnectivityIn", "ConnectivityOut",
                                               "PreLRMSWaitingJobs", "RunningJobs", "StagingJobs",
                                               "SuspendedJobs", "TotalJobs", "WaitingJobs",
                                               "Implementor", "ImplementationName", "ImplementationVersion", "QualityLevel",
                                               "ServingState", "Type" );
            $expected_names["job"] = array("ID", "Owner", "LocalOwner", "Error", "ComputingShareID",
                                           "IDFromEndpoint", "JobDescription", "ProxyExpirationTime",
                                           "Queue", "RequestedSlots", "RestartState", "State", "StdErr",
                                           "StdIn", "StdOut", "SubmissionHost", "SubmissionTime", "Type",
                                           "WorkingAreaEraseTime");
            $expected_names["queue"] = array("ID", "ComputingEndpointID", "ExecutionEnvironmentID", "Description",
                                             "FreeSlots", "FreeSlotsWithDuration", "LocalRunningJobs",
                                             "LocalSuspendedJobs", "LocalWaitingJobs", "Rule", "Scheme",
                                             "MappingQueue", "MaxCPUTime", "MaxRunningJobs", "MaxSlotsPerJob",
                                             "MaxVirtualMemory", "MaxWallTime", "MinCPUTime", "MinWallTime",
                                             "Name", "PreLRMSWaitingJobs", "RequestedSlots", "RunningJobs",
                                             "ServingState", "StagingJobs", "SuspendedJobs", "TotalJobs",
                                             "UsedSlots", "WaitingJobs");
            $app_count = 0;
            foreach ($expected_names[$scope] as $expected_name) {
                if (isset($this->result[$expected_name])) {
                    $drowcont[0] = $expected_name;
                    if ( $expected_name == "AppName") {
                        $drowcont[0] = "ApplicationEnvironments";
                        $drowcont[1] = "<textarea readonly name=\"l\" rows=\"4\" cols=\"60\" style=\"font-size:small\">\n";
                        foreach ($this->result[$expected_name] as $line){
                            $drowcont[1] .= $line . "-";
                            $drowcont[1] .= $this->result["AppVersion"][$app_count] . "   @ State: ";
                            $drowcont[1] .= $this->result["State"][$app_count] . "\n";
                            $app_count++;
                        }
                        $drowcont[1] .= "</textarea>";
                    } else if (count($this->result[$expected_name]) == 1) {
                        $drowcont[1] = $this->result[$expected_name][0];
                    } else if (count($this->result[$expected_name]) > 1) {
                        $drowcont[1] = "<textarea readonly name=\"l\" rows=\"4\" cols=\"60\" style=\"font-size:small\">\n";
                        $first_value = $this->result[$expected_name][0];
                        $all_elements_equal = true;
                        foreach ($this->result[$expected_name] as $line){
                            // All elements are equal or not
                            if ( $all_elements_equal && $first_value != $line ) {
                                $all_elements_equal = false;
                            }
                            $drowcont[1] .= $line . "\n";
                        }
                        $drowcont[1] .= "</textarea>";
                        // If all elements equal, then show only one of them.
                        if ( $all_elements_equal == true ) $drowcont[1] = $first_value;
                    } else if (count($this->result[$expected_name]) < 1) {
                        $drowcont[1] = "&nbsp;";
                    }
                    if ($expected_names[$scope][0] == $expected_name) {
                        $drowcont[0] = "<b>".$drowcont[0]."</b>";
                        $dtable->addrow($drowcont, "#cccccc");
                    } else $dtable->addrow($drowcont);
                }
            }
            $dtable->close();
        }
    }

}


?>
