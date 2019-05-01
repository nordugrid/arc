#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ApelDestination.h"

#include <sys/stat.h>

#include "jura.h"
#include <arc/Utils.h>
#include <arc/communication/ClientInterface.h>
#include <sstream>
#include <string> 

namespace ArcJura
{
  // Construct APEL destination during republishing using provided URL and topic
  ApelDestination::ApelDestination(std::string url_, std::string topic_):
    logger(Arc::Logger::rootLogger, "JURA.ApelReReporter"),
    rereport(true),
    use_ssl("false"),
    urn(0),
    sequence(0),
    usagerecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/computerecord"),
                   "UsageRecords")

  {
    // define APEL URL stripping 'APEL:' if present
    std::string apel_url = url_;
    if (url_.substr(0,5) == "APEL:") {
        apel_url = url_.substr(5);
    }
    // check SSL url
    if (apel_url.substr(0,5) == "https") {
        use_ssl = "true";
    }
    // NOTE that cert/key/cadir path will be read from environment variables or defaults are used
    init(apel_url, topic_, "", "", "", "");
  }

  // Construct APEL destination during normal publishing cycle from joblog and arc.conf
  ApelDestination::ApelDestination(JobLogFile& joblog, const Config::APEL &_conf):
    logger(Arc::Logger::rootLogger, "JURA.ApelDestination"),
    conf(_conf),
    rereport(false),
    use_ssl("false"),
    urn(0),
    sequence(0),
    usagerecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/computerecord"),
                   "UsageRecords")

  {
    // WARNING: 'loggerurl' should contains 'APEL:' prefix.
    // Jura adds this prefix when original A-REX joblogs are converted to per-destination joblogs in accordance to configuration in arc.conf
    init(joblog["loggerurl"].substr(5), joblog["topic"], joblog["outputdir"], joblog["certificate_path"], joblog["key_path"], joblog["ca_certificates_dir"]);

    //From jobreport_options:
    max_ur_set_size=conf.urbatchsize;
    use_ssl=conf.use_ssl;
  }

  void ApelDestination::init(std::string serviceurl_,std::string topic_, std::string outputdir_, std::string cert_, std::string key_, std::string ca_)
  {
    //Get service URL, cert, key, CA path from job log file
    std::string serviceurl=serviceurl_;
    topic=topic_;
    output_dir=outputdir_;
    std::string certfile=cert_;
    std::string keyfile=key_;
    std::string cadir=ca_;
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
            logger.msg(Arc::ERROR, "Protocol is %s. It is recommended to use secure connection with https.",
                       url.Protocol());
          }
        host=url.Host();
        std::ostringstream os;
        os<<url.Port();
        port=os.str();
        endpoint=url.Path();
      }

    //read the previous aggregation records
    if (!rereport)
        aggregationManager = new CARAggregation(host,port,topic, true);

    //Get Batch Size:
    //Default value:
    max_ur_set_size=JURA_DEFAULT_MAX_APEL_UR_SET_SIZE;
  }

  void ApelDestination::report(JobLogFile &joblog)
  {
    //if (joblog.exists())
      {
        //Store copy of job log
        joblogs.push_back(joblog);
        //Create UR if can
        Arc::XMLNode usagerecord(Arc::NS(), "");
        joblog.createCARUsageRecord(usagerecord);
        if (usagerecord)
          {
            usagerecordset.NewChild(usagerecord);
            ++urn;
            aggregationManager->UpdateAggregationRecord(usagerecord);
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

  void ApelDestination::report(std::string &joblog)
  {
    //Create UR if can
    //Arc::XMLNode usagerecord(Arc::NS(), "");
    Arc::XMLNode usagerecord;
    usagerecord.ReadFromFile(joblog);
    if (usagerecord)
      {
        usagerecordset.NewChild(usagerecord);
        ++urn;
        //aggregationManager->UpdateAggregationRecord(usagerecord);
      }
    else
      {
        logger.msg(Arc::INFO,"Ignoring incomplete log file \"%s\"",
                   joblog.c_str());
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
    if (!rereport)
        delete aggregationManager;
  }

  int ApelDestination::submit_batch()
  {
    std::string urstr;
    usagerecordset.GetDoc(urstr,false);

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
        log_sent_ids(usagerecordset, urn, logger, "APEL");
        if (!rereport) {
            // Save the modified aggregation records
            aggregationManager->save_records();
            // Reported the new synch record
            aggregationManager->Reporting_records();
        }

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
    std::string output_filename = Current_Time();
    char chars[] = ".-+T:";
    for (unsigned int i = 0; i < strlen(chars); ++i) {
        output_filename.erase (
                        std::remove(output_filename.begin(),
                                    output_filename.end(),
                                    chars[i]),
                        output_filename.end());
    }
    output_filename=output_filename.substr(0,14);

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
    std::string default_path = (std::string)JURA_DEFAULT_DIR_PREFIX + "/ssm/";
    struct stat st;
    //directory check
    if (stat(default_path.c_str(), &st) != 0) {
        mkdir(default_path.c_str(), S_IRWXU);
    }
    //directory check (for host)
    std::string subdir = default_path + service_url.Host() + "/";
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
    //ssmsend -H <hostname> -p <port> -t <topic> -k <key path> -c <cert path> -C <cadir path> -m <messages path> [--ssl]"
    std::string command = INSTPREFIX "/" PKGLIBEXECSUBDIR "/ssmsend";

    command += " -H " + service_url.Host(); //host
    std::stringstream port;
    port << service_url.Port();
    command += " -p " + port.str(); //port
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
    logger.msg(Arc::DEBUG, "SSM client exit code: %d", retval);
    if (retval == 0) {
        return Arc::MCC_Status(Arc::STATUS_OK,
                               "apelclient",
                               "APEL message sent.");
    } else {
        return Arc::MCC_Status(Arc::GENERIC_ERROR,
                               "apelclient",
                               "Some error has during the APEL message sending. \
                                See SSM log (/var/spool/arc/ssm/ssmsend.log) for more details.");
    }
  }

  void ApelDestination::clear()
  {
    urn=0;
    joblogs.clear();
    usagerecordset.Replace(
        Arc::XMLNode(Arc::NS("",
                             "http://eu-emi.eu/namespaces/2012/11/computerecord"
                            ),
                     "UsageRecords")
                    );
  }

  ApelDestination::~ApelDestination()
  {
    finish();
  }

}
