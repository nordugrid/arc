#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "CARDestination.h"

#include "jura.h"
#include <arc/Utils.h>
#include <arc/communication/ClientInterface.h>
#include <sstream>

namespace Arc
{
  CARDestination::CARDestination(JobLogFile& joblog):
    logger(Arc::Logger::rootLogger, "JURA.CARDestination"),
    urn(0),
    usagerecordset(Arc::NS("","http://eu-emi.eu/namespaces/2012/11/computerecord"),
                   "UsageRecords")
  {
    //Get service URL, cert, key, CA path from job log file
    std::string serviceurl=joblog["loggerurl"].substr(4);
    std::string certfile=joblog["certificate_path"];
    std::string keyfile=joblog["key_path"];
    std::string cadir=joblog["ca_certificates_dir"];
    output_dir=joblog["outputdir"];
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
    max_ur_set_size=1000;
    //From jobreport_options:
    std::string urbatch=joblog["jobreport_option_urbatch"];
    if (!urbatch.empty())
      {
        std::istringstream is(urbatch);
        is>>max_ur_set_size;
      }

  }

  void CARDestination::report(Arc::JobLogFile &joblog)
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

  void CARDestination::finish()
  {
    if (urn>0)
      // Send the remaining URs and delete job log files.
      submit_batch();
  }

  int CARDestination::submit_batch()
  {
    std::string urstr;
    usagerecordset.GetDoc(urstr,false);

    logger.msg(Arc::INFO, 
               "Logging UR set of %d URs.",
               urn);
    logger.msg(Arc::DEBUG, 
               "UR set dump: %s",
               urstr.c_str());
    
    return OutputFileGeneration("CAR", service_url, output_dir, urstr, logger);
  }

  Arc::MCC_Status CARDestination::send_request(const std::string &urset)
  {
    //TODO: Not implemented yet.
    return Arc::MCC_Status(Arc::STATUS_OK,
                           "carclient",
                           "CAR message sent.");
  }

  void CARDestination::clear()
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

  CARDestination::~CARDestination()
  {
    finish();
  }
}
