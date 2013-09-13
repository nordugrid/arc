#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include "CARAggregation.h"

#include <sys/stat.h>

#include "jura.h"
#include "Destination.h"
#include <arc/Utils.h>

namespace Arc
{
  CARAggregation::CARAggregation(std::string _host):
    logger(Arc::Logger::rootLogger, "JURA.CARAggregation"),
    aggr_record_update_need(false),
    aggregationrecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords")
  {
    CARAggregation(_host, "", "");
  }

  CARAggregation::CARAggregation(std::string _host, std::string _port, std::string _topic):
    logger(Arc::Logger::rootLogger, "JURA.CARAggregation"),
    use_ssl("false"),
    sequence(0),
    aggr_record_update_need(false),
    aggregationrecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord"),
                   "SummaryRecords")

  {
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
        if (aggr_record_update_need)
          {
            if (aggregationrecordset.SaveToFile(aggr_record_location))
              {
                logger.msg(Arc::INFO, "Aggregation record (%s) stored successful.",
                           aggr_record_location);
              }
            else
              {
                logger.msg(Arc::ERROR, "Some error happens during the Aggregation record (%s) storing.",
                           aggr_record_location);
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
    //ssmsend <hostname> <port> <topic> <key path> <cert path> <cadir path> <messages path> <use_ssl>"
    std::string command;
    std::vector<std::string> ssm_pathes;
    std::string exec_cmd = "ssmsend";
    //RedHat: /usr/libexec/arc/ssm_master
    ssm_pathes.push_back("/usr/libexec/arc/"+exec_cmd);
    ssm_pathes.push_back("/usr/local/libexec/arc/"+exec_cmd);
    // Ubuntu/Debian: /usr/lib/arc/ssm_master
    ssm_pathes.push_back("/usr/lib/arc/"+exec_cmd);
    ssm_pathes.push_back("/usr/local/lib/arc/"+exec_cmd);

    // If you don't use non-standard prefix for a compilation you will 
    // use this extra location.
    std::ostringstream prefix;
    prefix << INSTPREFIX << "/" << PKGLIBEXECSUBDIR << "/";
    ssm_pathes.push_back(prefix.str()+exec_cmd);
    
    // Find the location of the ssm_master
    std::string ssm_command = "./ssm/"+exec_cmd;
    for (int i=0; i<(int)ssm_pathes.size(); i++) {
        std::ifstream ssmfile(ssm_pathes[i].c_str());
        if (ssmfile) {
            // The file exists,
            ssm_command = ssm_pathes[i];
            ssmfile.close();
            break;
        }
    }

    command = ssm_command;
    command += " " + host;       //host
    command += " " + port;       //port
    command += " " + topic;      //topic
    command += " " + cfg.key;    //certificate key
    command += " " + cfg.cert;   //certificate
    command += " " + cfg.cadir;  //cadir
    command += " " + default_path; //messages path
    command += " " + use_ssl;    //use_ssl
    command += "";

    retval = system(command.c_str());
    logger.msg(Arc::DEBUG, "system retval: %d", retval);
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

    std::string query("//SummaryRecords/SummaryRecord[Year='");
    query += year;
    query += "' and Month='" + month;
    query += "' and Queue='" + queue;
    query += "']";
    logger.msg(Arc::DEBUG, "query: %s", query);

    Arc::NS ns;
    ns[""] = "http://eu-emi.eu/namespaces/2012/11/aggregatedcomputerecord";
    ns["urf"] = "http://eu-emi.eu/namespaces/2012/11/computerecord";
    Arc::XMLNodeList list = aggregationrecordset.XPathLookup(query,Arc::NS());
    logger.msg(Arc::DEBUG, "list size: %d", (int)list.size());
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
        new_node.NewChild("LastSending");
            
        // Add new node to the aggregation record collection
        aggregationrecordset.NewChild(new_node);
      }
    else {
        // Exist Aggregation record for this month, compare needed

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
        
        // WallDuration
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
        nrofjobs << ((int)node["NumberOfJobs"])+1;
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
    save_records();
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
}
