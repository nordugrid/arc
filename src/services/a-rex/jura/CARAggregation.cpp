#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include "CARAggregation.h"

#include <sys/stat.h>

#include "jura.h"
#include "Destination.h"
#include <arc/StringConv.h>
#include <arc/Utils.h>

namespace ArcJura
{
  CARAggregation::CARAggregation(std::string _host):
    logger(Arc::Logger::rootLogger, "JURA.CARAggregation"),
    aggr_record_update_need(false),
    synch_message(false),
    aggregationrecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords")
  {
    init(_host, "", "");
  }

  CARAggregation::CARAggregation(std::string _host, std::string _port, std::string _topic, bool synch):
    logger(Arc::Logger::rootLogger, "JURA.CARAggregation"),
    use_ssl("false"),
    sequence(0),
    aggr_record_update_need(false),
    synch_message(false),
    aggregationrecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords")
  {
    synch_message = synch;
    init(_host, _port, _topic);
  }

  void CARAggregation::init(std::string _host, std::string _port, std::string _topic)
  {
    ns[""] = "http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord";
    ns["urf"] = "http://eu-emi.eu/namespaces/2012/11/computerecord";
    
    ns_query["car"] = "http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord";
    ns_query["urf"] = "http://eu-emi.eu/namespaces/2012/11/computerecord";

    // Get cert, key, CA path from environment
    std::string certfile=Arc::GetEnv("X509_USER_CERT");
    std::string keyfile=Arc::GetEnv("X509_USER_KEY");
    std::string cadir=Arc::GetEnv("X509_CERT_DIR");
    // ...or by default, use host cert, key, CA path
    if (certfile.empty())
      certfile=JURA_DEFAULT_CERT_FILE;
    if (keyfile.empty())
      keyfile=JURA_DEFAULT_KEY_FILE;
    if (cadir.empty())
      cadir=JURA_DEFAULT_CA_DIR;

    cfg.AddCertificate(certfile);
    cfg.AddPrivateKey(keyfile);
    cfg.AddCADir(cadir);

    host  = _host;
    port  = _port;
    topic = _topic;
    //read the previous aggregation records
    std::string default_path = (std::string)JURA_DEFAULT_DIR_PREFIX + "/urs/";
    aggr_record_location = default_path + host + "_aggregation_records.xml";
    if (!aggregationrecordset.ReadFromFile(aggr_record_location))
      {
        logger.msg(Arc::INFO, "Aggregation record (%s) not exist, initialize it...",
                       aggr_record_location);
        if (aggregationrecordset.SaveToFile(aggr_record_location))
          {
            logger.msg(Arc::INFO, "Aggregation record (%s) initialization successful.",
                       aggr_record_location);
          }
        else
          {
            logger.msg(Arc::ERROR, "Some error happens during the Aggregation record (%s) initialization.",
                       aggr_record_location);
           }
      }
    else
      {
        logger.msg(Arc::DEBUG, "Aggregation record (%s) read from file successful.",
                       aggr_record_location);
       }
  }


  int CARAggregation::save_records()
  {
    // Save the stored aggregation records
    if (aggr_record_update_need) {
        if (aggregationrecordset.SaveToFile(aggr_record_location)) {
            aggr_record_update_need = false;
            logger.msg(Arc::INFO, "Aggregation record (%s) stored successful.",
                       aggr_record_location);
        } else {
            logger.msg(Arc::ERROR, "Some error happens during the Aggregation record (%s) storing.",
                       aggr_record_location);
            return 1;
        }
    }
    return 0;
  }

  Arc::MCC_Status CARAggregation::send_records(const std::string &urset)
  {
    //Filename generation
    std::string output_filename = Current_Time();
    char chars[] = "-+T:";
    for (unsigned int i = 0; i < strlen(chars); ++i) {
        output_filename.erase (
                        std::remove(output_filename.begin(),
                                    output_filename.end(),
                                    chars[i]),
                        output_filename.end());
    }
    output_filename=output_filename.substr(0,14);

    //Save all records into the default folder.
    std::string default_path = (std::string)JURA_DEFAULT_DIR_PREFIX + "/ssm/";
    struct stat st;
    //directory check
    if (stat(default_path.c_str(), &st) != 0) {
        mkdir(default_path.c_str(), S_IRWXU);
    }
    //directory check (for host)
    std::string subdir = default_path + host + "/";
    if (stat(subdir.c_str(), &st) != 0) {
        mkdir(subdir.c_str(), S_IRWXU);
    }
    //directory check
    subdir = subdir + "outgoing/";
    default_path = subdir.substr(0,subdir.length()-1);
    if (stat(subdir.c_str(), &st) != 0) {
        mkdir(subdir.c_str(), S_IRWXU);
    }
    subdir = subdir + "00000000/";
    if (stat(subdir.c_str(), &st) != 0) {
        mkdir(subdir.c_str(), S_IRWXU);
    }

    // create message file to the APEL client
    subdir += output_filename;
    const char* filename(subdir.c_str());
    std::ofstream outputfile;
    outputfile.open (filename);
    if (outputfile.is_open()) {
        outputfile << urset;
        outputfile.close();
        logger.msg(Arc::DEBUG, "APEL aggregation message file (%s) created.", output_filename);
    } else {
        return Arc::MCC_Status(Arc::PARSING_ERROR,
               "apelclient",
               std::string(
                 "Error opening file: "
                           )+ 
               filename
               );
    }
    int retval;
    //ssmsend -H <hostname> -p <port> -t <topic> -k <key path> -c <cert path> -C <cadir path> -m <messages path> [--ssl]"
    std::string command = INSTPREFIX + "/" + PKGLIBEXECSUBDIR + "/ssmsend";

    command += " -H " + host; //host
    command += " -p " + port; //port
    command += " -t " + topic;      //topic
    command += " -k " + cfg.key;    //certificate key
    command += " -c " + cfg.cert;   //certificate
    command += " -C " + cfg.cadir;  //cadir
    command += " -m " + default_path; //messages path
    command += " -d " + Arc::level_to_string(logger.getThreshold()); // loglevel
    if (use_ssl == "true") {
        command += " --ssl";    //use_ssl
    }
    command += "";

    logger.msg(Arc::INFO, "Running SSM client using: %s", command);
    retval = system(command.c_str());
    logger.msg(Arc::DEBUG, "SSM client return value: %d", retval);
    if (retval == 0) {
        return Arc::MCC_Status(Arc::STATUS_OK,
                               "apelclient",
                               "APEL message sent.");
    } else {
        return Arc::MCC_Status(Arc::GENERIC_ERROR,
                               "apelclient",
                               "Some error has during the APEL message sending.");
    }
  }

  void CARAggregation::UpdateAggregationRecord(Arc::XMLNode& ur)
  {
    std::string endtime = ur["EndTime"];
    std::string year = endtime.substr(0,4);
    std::string month = endtime.substr(5,2);

    std::string queue = ur["Queue"];

    logger.msg(Arc::DEBUG, "year: %s", year);
    logger.msg(Arc::DEBUG, "moth: %s", month);
    logger.msg(Arc::DEBUG, "queue: %s", queue);

    std::string query("//car:SummaryRecords/car:SummaryRecord[car:Year='");
    query += year;
    query += "' and car:Month='" + month;
    std::string queryPrefix(query);
    query += "' and Queue='" + queue;
    query += "']";
    logger.msg(Arc::DEBUG, "query: %s", query);

    Arc::XMLNodeList list = aggregationrecordset.XPathLookup(query,ns_query);
    logger.msg(Arc::DEBUG, "list size: %d", (int)list.size());
    if ( list.size() == 0 ) {
       // When read XML from file create a namespace for the Queue element.    
       query = queryPrefix + "' and car:Queue='" + queue + "']";
       list = aggregationrecordset.XPathLookup(query,ns_query);
    }
    /**
     * CAR aggregation record elements:
     *   Site*
     *   Month*
     *   Year*
     *   UserIdentity
     *   SubmitHost**
     *   Host
     *   Queue
     *   Infrastructure*
     *   Middleware
     *   EarliestEndTime
     *   LatestEndTime
     *   WallDuration*
     *   CpuDuration*
     *   ServiceLevel*
     *   NumberOfJobs*
     *   Memory
     *   Swap
     *   NodeCount
     *   Processors
     * 
     * notes: *  mandatory for CAR
     *        ** mandatory for APEL synch
     * 
     */
    if ( list.empty())
      {
        // Not exist Aggregation record for this month
        Arc::XMLNode new_node(ns,"SummaryRecord");
        new_node.NewChild(ur["Site"]);
        new_node.NewChild("Year") = year;
        new_node.NewChild("Month") = month;
        new_node.NewChild(ur["UserIdentity"]);
        new_node["UserIdentity"]["LocalUserId"].Destroy();
        new_node.NewChild(ur["SubmitHost"]);
        new_node.NewChild(ur["Host"]);
        new_node.NewChild(ur["Queue"]);
        new_node.NewChild(ur["Infrastructure"]);
        new_node.NewChild(ur["Middleware"]);
        new_node.NewChild("EarliestEndTime") = endtime;
        new_node.NewChild("LatestEndTime") = endtime;
        new_node.NewChild(ur["WallDuration"]);
        new_node.NewChild(ur["CpuDuration"]);
        new_node.NewChild(ur["ServiceLevel"]);
        new_node.NewChild("NumberOfJobs") = "1";
        //new_node.NewChild("Memory");
        //new_node.NewChild("Swap");
        new_node.NewChild(ur["NodeCount"]);
        //new_node.NewChild("Processors");
        
        // Local informations
        // LastModification
        new_node.NewChild("LastModification") = Current_Time();
        // LastSending
            
        // Add new node to the aggregation record collection
        aggregationrecordset.NewChild(new_node);
      }
    else {
        // Aggregation record exist for this month, comparison needed
        Arc::XMLNode node = list.front();
        // EarliestEndTime
        std::string endtime = ur["EndTime"];
        if ( endtime.compare(node["EarliestEndTime"]) < 0 ) {
            node["EarliestEndTime"] = endtime;
        }

        // LatestEndTime
        if ( endtime.compare(node["LatestEndTime"]) > 0 ) {
            node["LatestEndTime"] = endtime;
        }

        // WallDuration
        Arc::Period walldur((std::string)node["WallDuration"]);       
        Arc::Period new_walldur((std::string)ur["WallDuration"]);
        walldur+=new_walldur;
        std::string walld = (std::string)walldur;
        if (walld == "P"){
          walld = "PT0S";
        }
        node["WallDuration"] = walld;
        
        // CpuDuration
        Arc::Period cpudur((std::string)node["CpuDuration"]);       
        Arc::Period new_cpudur((std::string)ur["CpuDuration"]);
        cpudur+=new_cpudur;
        std::string cpud = (std::string)cpudur;
        if (cpud == "P"){
          cpud = "PT0S";
        }
        node["CpuDuration"] = cpud;

        // NumberOfJobs
        std::ostringstream nrofjobs;
        nrofjobs << Arc::stringtoi(((std::string)node["NumberOfJobs"]))+1;
        node["NumberOfJobs"] =  nrofjobs.str();

        //node.NewChild("Memory");
        //node.NewChild("Swap");
        //node.NewChild("NodeCount");
        //node.NewChild("Processors");

        // Local informations
        // LastModification
        node["LastModification"] = Current_Time();
      }
    aggr_record_update_need = true;

    { //only DEBUG information
    std::string ss;
    aggregationrecordset.GetXML(ss,true);
    logger.msg(Arc::DEBUG, "XML: %s", ss);
    }
    logger.msg(Arc::DEBUG, "UPDATE Aggregation Record called.");
  }

  CARAggregation::~CARAggregation()
  {
    //save_records();
  }

  // Current time calculation and convert to the UTC time format.
  std::string CARAggregation::Current_Time( time_t parameter_time ){

      time_t rawtime;
      if ( parameter_time == time(NULL) ){
          time ( &rawtime );    //current time
      } else {
          rawtime = parameter_time;
      }
      tm * ptm;
      ptm = gmtime ( &rawtime );

      std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
      std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
      std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
      std::string min_prefix = (ptm->tm_min < 10)?"0":"";
      std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
      std::stringstream out;
      if ( parameter_time == time(NULL) ){
          out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T"<<hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec<<"+0000";
      } else {
          out << ptm->tm_year+1900<<mon_prefix<<ptm->tm_mon+1<<day_prefix<<ptm->tm_mday<<"."<<hour_prefix<<ptm->tm_hour<<min_prefix<<ptm->tm_min<<sec_prefix<<ptm->tm_sec;
      }
      return out.str();
  }
  
  bool CARAggregation::Reporting_records(std::string year, std::string month)
  {
    // get the required records
    std::string query("//car:SummaryRecords/car:SummaryRecord[car:Year='");
    query += year;
    if(!month.empty()){
      query += "' and car:Month='" + month;
    }
    query += "']";
    logger.msg(Arc::DEBUG, "query: %s", query);

    Arc::XMLNodeList list = aggregationrecordset.XPathLookup(query,ns_query);

    Arc::XMLNode sendingXMLrecords(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords");
    for(Arc::XMLNodeList::iterator liter = list.begin(); liter != list.end(); ++liter) {
      NodeCleaning(sendingXMLrecords.NewChild(*liter));
    }

    if ( sendingXMLrecords.Size() == 0 ) {
      logger.msg(Arc::INFO, "Does not sending empty aggregation/synch message.");
      return true;
    }
    // send the required records
    std::string records;
    if (synch_message) {
      //Synch record need to be send
      records = SynchMessage(sendingXMLrecords);
    } else {
      //Aggregation record need to be send
      sendingXMLrecords.GetXML(records,true);
    }
    Arc::MCC_Status status = send_records(records);
    if ( status != Arc::STATUS_OK ) {
      return false;
    }
    // Update the last sending dates
    UpdateLastSendingDate(list);
    // Store the records
    return save_records();
  }

  bool CARAggregation::Reporting_records(bool force_resend)
  {
    Arc::XMLNode sendingXMLrecords(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords");

    Arc::XMLNode node = aggregationrecordset["SummaryRecord"];
    while (node) {
      if ( force_resend ) {
        // force resend all records
        NodeCleaning(sendingXMLrecords.NewChild(node));
        ++node;
        continue;
      }
      Arc::XMLNode lastsendingnode = node["LastSending"];
      std::string lastsending= "";
      // find the latest LastSending element
      while ( lastsendingnode ) {
        lastsending = (std::string)lastsendingnode;
        ++lastsendingnode;
      }
      // collect all modified records
      if ( lastsending < (std::string)node["LastModification"] ) {
        NodeCleaning(sendingXMLrecords.NewChild(node));
      }
      ++node;
    }

    if ( sendingXMLrecords.Size() == 0 ) {
      logger.msg(Arc::INFO, "Does not sending empty aggregation/synch message.");
      return true;
    }
    // send all records
    std::string all_records;
    if (synch_message) {
      //Synch record need to be send
      all_records = SynchMessage(sendingXMLrecords);
    } else {
      //Aggregation record need to be send
      sendingXMLrecords.GetXML(all_records,true);
    }
    Arc::MCC_Status status = send_records(all_records);
    if ( status != Arc::STATUS_OK ) {
      return false;
    }
    // Update the last sending dates
    UpdateLastSendingDate();
    // Store the records
    return save_records();
  }
  
  void CARAggregation::UpdateLastSendingDate()
  {
    std::string query("//car:SummaryRecords/car:SummaryRecord");
    
    Arc::XMLNodeList list = aggregationrecordset.XPathLookup(query,ns_query);

    UpdateLastSendingDate(list);
  }

  void CARAggregation::UpdateLastSendingDate(Arc::XMLNodeList& records)
  {
    std::string current_time = Current_Time();
    for(Arc::XMLNodeList::iterator liter = records.begin(); liter != records.end(); ++liter) {
      UpdateElement( *liter, "LastSending", current_time);
    }
    if(records.size() > 0){
      aggr_record_update_need = true;
    }
  }

  void CARAggregation::UpdateElement(Arc::XMLNode& node, std::string name, std::string time) {
    Arc::XMLNode mod_node = node[name];
    //find latest date
    Arc::XMLNode cnode = node[name];
    for (int i=0; i<10; i++) {
     if (cnode) {
        if ( std::string(cnode) < std::string(mod_node)) {
          mod_node = cnode;
          break;
        }
      } else {
		// create a new child
		mod_node = node.NewChild(name);
		break;
	  }
	  ++cnode;
    }
    //update the element
    mod_node = time;
  }

  std::string CARAggregation::SynchMessage(Arc::XMLNode records)
  {
    std::string result;
    //header
    result = "APEL-sync-message: v0.1\n";
    Arc::XMLNode node = records["SummaryRecord"];
    while (node) {
      //Site
      result += "Site: " + (std::string)node["Site"] + "\n";
      //SubmitHost
      result += "SubmitHost: " + (std::string)node["SubmitHost"] + "/" + (std::string)node["Queue"] + "\n";
      //NumberOfJobs
      result += "NumberOfJobs: " + (std::string)node["NumberOfJobs"] + "\n";
      //Month
      result += "Month: " + (std::string)node["Month"] + "\n";
      //Year
      result += "Year: " + (std::string)node["Year"] + "\n";
      result += "%%\n";
      ++node;
    }
    logger.msg(Arc::DEBUG, "synch message: %s", result);
    return result;
  }
  
  void CARAggregation::NodeCleaning(Arc::XMLNode node)
  {
    /** Remove the local information from the sending record.
     *  These attributes are not CAR related values.
     */
    node["LastModification"].Destroy();
    Arc::XMLNode next = (Arc::XMLNode)node["LastSending"];
    while (next) {
      Arc::XMLNode prev = next;
      ++next;
      prev.Destroy();
    }
  }
}
