#include "ApelDestination.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>

#include "jura.h"
#include <arc/Utils.h>
#include <arc/client/ClientInterface.h>
#include <sstream>

namespace Arc
{
  ApelDestination::ApelDestination(JobLogFile& joblog):
    logger(Arc::Logger::rootLogger, "JURA.ApelDestination"),
    urn(0),
    sequence(0)
  {
    //Get service URL, cert, key, CA path from job log file
    std::string serviceurl=joblog["loggerurl"];
    topic=joblog["topic"];
    output_dir=joblog["outputdir"];
    std::string certfile=joblog["certificate_path"];
    std::string keyfile=joblog["key_path"];
    std::string cadir=joblog["ca_certificates_dir"];
    // ...or get them from environment
    if (certfile.empty())
      certfile=Arc::GetEnv("X509_USER_CERT");
    if (keyfile.empty())
      keyfile=Arc::GetEnv("X509_USER_KEY");
    if (cadir.empty())
      cadir=Arc::GetEnv("X509_CERT_DIR");
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

    //  Tokenize service URL
    std::string host, port, endpoint;
    if (serviceurl.empty())
      {
        logger.msg(Arc::ERROR, "ServiceURL missing");
      }
    else
      {
        Arc::URL url(serviceurl);
        service_url = url;
        if (url.Protocol()!="https")
          {
            logger.msg(Arc::ERROR, "Protocol is %s, should be https",
                       url.Protocol());
          }
        host=url.Host();
        std::ostringstream os;
        os<<url.Port();
        port=os.str();
        endpoint=url.Path();
      }

    //Get Batch Size:
    //Default value:
    max_ur_set_size=JURA_DEFAULT_MAX_APEL_UR_SET_SIZE;
    //From jobreport_options:
    std::string urbatch=joblog["jobreport_option_urbatch"];
    if (!urbatch.empty())
      {
         std::istringstream is(urbatch);
        is>>max_ur_set_size;
      }

  }

  void ApelDestination::report(Arc::JobLogFile &joblog)
  {
    //if (joblog.exists())
      {
        //Store copy of job log
        joblogs.push_back(joblog);
        //Create UR if can
        Arc::XMLNode usagerecord(Arc::NS(), "");
        joblog.createUsageRecord(usagerecord);
        if (usagerecord)
          {
            std::map<std::string, std::string> usagerecord_apel;
            XML2KeyValue(usagerecord, usagerecord_apel);
            usagerecordset_apel.push_back(usagerecord_apel);
            ++urn;
          }
        else
          {
            logger.msg(Arc::INFO,"Ignoring incomplete log file \"%s\"",
                       joblog.getFilename().c_str());
            joblog.remove();
          }
      }

    if (urn==max_ur_set_size)
      // Batch is full. Submit and delete job log files.
      submit_batch();
  }

  void ApelDestination::finish()
  {
    if (urn>0)
      // Send the remaining URs and delete job log files.
      submit_batch();
  }

  int ApelDestination::submit_batch()
  {
    std::string urstr;
    urstr = "APEL-individual-job-message: v0.2\n";
    std::map<std::string,std::string>::iterator it;
    for (int i=0; i<(int)usagerecordset_apel.size(); i++){
        for ( it=usagerecordset_apel[i].begin() ; it != usagerecordset_apel[i].end(); it++ ){
            urstr += (*it).first + ": " + (*it).second + "\n";
        }
        urstr += "%%\n";
    }

    logger.msg(Arc::INFO, 
               "Logging UR set of %d URs.",
               urn);
    logger.msg(Arc::DEBUG, 
               "UR set dump: %s",
               urstr.c_str());
  
    // Communication with Apel server
    Arc::MCC_Status status=send_request(urstr);

    if (status.isOk())
      {
        // Delete log files
        for (std::list<JobLogFile>::iterator jp=joblogs.begin();
             jp!=joblogs.end();
             jp++
             )
          {
            (*jp).remove();
          }
        clear();
        return 0;
      }
    else // status.isnotOk
      {
        logger.msg(Arc::ERROR, 
                   "%s: %s",
                   status.getOrigin().c_str(),
                   status.getExplanation().c_str()
                   );
        clear();
        return -1;
      }
  }

  Arc::MCC_Status ApelDestination::send_request(const std::string &urset)
  {
    //Filename generation
    std::string output_filename = "Apel_records_" +service_url.Host() + "_" + Current_Time();
    if (!output_dir.empty()) {
        // local copy creation
        std::string output_path;
        output_path = output_dir;
        if (output_dir[output_dir.size()-1] != '/'){
            output_path = output_dir + "/";
        }
    
    
        output_path += output_filename;

        std::ifstream ifile(output_path.c_str());
        if (ifile) {
            // The file exists, and create new filename
            sequence++;
            std::stringstream ss;
            ss << sequence;
            output_path += ss.str();
            output_filename += ss.str();
        }
        else {
            sequence=0;
        }
        ifile.close();

        //Save all records into the output file.
        const char* filename(output_path.c_str());
        std::ofstream outputfile;
        outputfile.open (filename);
        if (outputfile.is_open())
        {
            outputfile << urset;
            outputfile.close();
            logger.msg(Arc::DEBUG, "Backup file (%s) created.", output_filename);
        }
        else
        {
            return Arc::MCC_Status(Arc::PARSING_ERROR,
                   "apelclient",
                   std::string(
                     "Error opening file: "
                               )+ 
                   filename
                   );
        }
    }

    //Save all records into the default folder.
    std::string default_path = "/tmp/messages/";
    struct stat st;
    //directory check
    if (stat(default_path.c_str(), &st) != 0) {
        mkdir(default_path.c_str(), S_IRWXU);
    }
    //directory check
    std::string subdir = default_path + "outgoing/";
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
        logger.msg(Arc::DEBUG, "APEL message file (%s) created.", output_filename);
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
    //ssm-master <path-to-config-file> <hostname> <port> <topic> <key path> <cert path> <cadir path> <messages path>"
    std::string command;
    std::vector<std::string> ssm_pathes;
    std::string exec_cmd = "ssm_master";
    //RedHat: /usr/libexec/arc/ssm_master
    ssm_pathes.push_back("/usr/libexec/arc/"+exec_cmd);
    ssm_pathes.push_back("/usr/local/libexec/arc/"+exec_cmd);
    // Ubuntu/Debian: /usr/lib/arc/ssm_master
    ssm_pathes.push_back("/usr/lib/arc/"+exec_cmd);
    ssm_pathes.push_back("/usr/local/lib/arc/"+exec_cmd);
    
    // Find the location of the ssm_master
    std::string ssm_command = "./ssm/"+exec_cmd;
    for (int i=0; i<ssm_pathes.size(); i++) {
        std::ifstream ssmfile(ssm_pathes[i].c_str());
        if (ssmfile) {
            // The file exists,
            ssm_command = ssm_pathes[i];
            ssmfile.close();
            break;
        }
    }

    command = ssm_command;
    command += " ssm.cfg"; //config
    command += " " + service_url.Host(); //host
    std::stringstream port;
    port << service_url.Port();
    command += " " + port.str(); //port
    command += " " + topic;      //topic
    command += " " + cfg.key;    //certificate key
    command += " " + cfg.cert;   //certificate
    command += " " + cfg.cadir;  //cadir
    command += " " + default_path; //messages path
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

  void ApelDestination::clear()
  {
    urn=0;
    joblogs.clear();
    usagerecordset_apel.clear();
  }

  ApelDestination::~ApelDestination()
  {
    finish();
  }

  void ApelDestination::XML2KeyValue(Arc::XMLNode &xml_ur, std::map<std::string, std::string> &key_ur)
  {
    //RecordIdentity 
    //GlobalJobId 

    //LocalJobId
    if( bool(xml_ur["JobIdentity"]["LocalJobId"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("LocalJobId", xml_ur["JobIdentity"]["LocalJobId"]) );
    }

    //ProcessId 

    //LocalUserId 
    if( bool(xml_ur["JobIdentity"]["LocalUserId"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("LocalUserId", xml_ur["JobIdentity"]["LocalUserId"]) );
    } 

    //GlobalUserName 
    if( bool(xml_ur["UserIdentity"]["GlobalUserName"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("GlobalUserName", xml_ur["UserIdentity"]["GlobalUserName"]) );
    } 
    //JobName 
    //Charge 
    //Status 

    //WallDuration 
    if( bool(xml_ur["WallDuration"]) ){
        std::stringstream ss;
        ss << Arc::Period(xml_ur["WallDuration"],Arc::PeriodSeconds).GetPeriod();
        key_ur.insert ( std::pair<std::string,std::string>("WallDuration", ss.str()) );
    } 

    //CpuDuration 
    if( bool(xml_ur["CpuDuration"]) ){
        int CpuDur(0);
        //CpuDuration = CpuDuration(kernel) + CpuDuration(user)
        for (int i=0; bool(xml_ur["CpuDuration"][i]); i++){
            CpuDur += (Arc::Period(xml_ur["CpuDuration"][i],Arc::PeriodSeconds)).GetPeriod();
        }
        std::stringstream ss;
        ss << CpuDur;
        key_ur.insert ( std::pair<std::string,std::string>("CpuDuration", ss.str()) );
    } 

    //StartTime 
    if( bool(xml_ur["StartTime"]) ){
        std::stringstream ss;
        ss << Arc::Time((std::string)xml_ur["StartTime"]).GetTime();
        key_ur.insert ( std::pair<std::string,std::string>("StartTime", ss.str()) );
    } 

    //EndTime 
    if( bool(xml_ur["EndTime"]) ){
        std::stringstream ss;
        ss << Arc::Time((std::string)xml_ur["EndTime"]).GetTime();
        key_ur.insert ( std::pair<std::string,std::string>("EndTime", ss.str()) );
    } 

    //MachineName 
    if( bool(xml_ur["MachineName"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("Site", xml_ur["MachineName"]) );
    } 

    //Host 
    //SubmitHost 
    //Queue
    //Headnode 
    if( bool(xml_ur["Headnode"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("SubmitHost", xml_ur["Headnode"]) );
    } 

    //ProjectName 
    //Network 
    //Disk 

    //Memory 
    if( bool(xml_ur["Memory"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("MemoryReal", xml_ur["Memory"]) );
    } 
 
    //Swap

    //Nodecount 
    if( bool(xml_ur["NodeCount"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("NodeCount", xml_ur["NodeCount"]) );
    } 

    //Processors 
    if( bool(xml_ur["Processors"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("Processors", xml_ur["Processors"]) );
    } 
    //TimeDuration
    //TimeInstant
    //ServiceLevel

    //Apel's default values
    key_ur.insert ( std::pair<std::string,std::string>("ServiceLevelType", "custom") );
    key_ur.insert ( std::pair<std::string,std::string>("ServiceLevel", "1") );
    key_ur.insert ( std::pair<std::string,std::string>("FQAN", "None") );
  }

}
