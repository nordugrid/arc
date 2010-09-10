#include "LutsDestination.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"
#include "arc/Utils.h"
#include "arc/client/ClientInterface.h"
#include <sstream>

namespace Arc
{
  LutsDestination::LutsDestination(JobLogFile& joblog):
    logger(Arc::Logger::rootLogger, "JURA.LutsDestination"),
    urn(0),
    usagerecordset(Arc::NS("","http://schema.ogf.org/urf/2003/09/urf"),
                   "UsageRecords")
  {
    //Get service URL, cert, key, CA path from job log file
    std::string serviceurl=joblog["loggerurl"];
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
    max_ur_set_size=JURA_DEFAULT_MAX_UR_SET_SIZE;
    //From jobreport_options:
    std::string urbatch=joblog["jobreport_option_urbatch"];
    if (!urbatch.empty())
      {
	std::istringstream is(urbatch);
	is>>max_ur_set_size;
      }

  }

  void LutsDestination::report(Arc::JobLogFile &joblog)
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

  void LutsDestination::finish()
  {
    if (urn>0)
      // Send the remaining URs and delete job log files.
      submit_batch();
  }

  int LutsDestination::submit_batch()
  {
    std::string urstr;
    usagerecordset.GetDoc(urstr,false);

    logger.msg(Arc::INFO, 
               "Logging UR set of %d URs.",
               urn);
    logger.msg(Arc::DEBUG, 
               "UR set dump: %s",
               urstr.c_str());
  
    // Communication with LUTS server
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

  Arc::MCC_Status LutsDestination::send_request(const std::string &urset)
  {
    ClientHTTP httpclient(cfg, service_url);
    //TODO: Absolute or relative url was in the configuration?
    httpclient.RelativeURI(true);

    PayloadRaw http_request;
    PayloadRawInterface *http_response = NULL;
    HTTPClientInfo http_info;
    std::multimap<std::string, std::string> http_attributes;
    Arc::MCC_Status status;

    //Add the message into the request
    http_request.Insert(urset.c_str());
    
    try
    {
      //Send
      status=httpclient.process("POST", http_attributes, &http_request, &http_info, &http_response);
      logger.msg(Arc::DEBUG, 
               "UsageRecords registration response: %s",
               http_response->Content());
     }
    catch (std::exception&) {}

    if (http_response==NULL)
      {
        //Unintelligible non-HTTP response
        return Arc::MCC_Status(Arc::PROTOCOL_RECOGNIZED_ERROR,
                               "lutsclient",
                               "Response not HTTP");
      }

    if (status && ((std::string)http_response->Content()).substr(0,1) != "{" )
      {
        // Status OK, but some error
        std::string httpfault;
        httpfault = http_response->Content();

        delete http_response;
        return Arc::MCC_Status(Arc::PARSING_ERROR,
               "lutsclient",
               std::string(
                 "Response from the server: "
                           )+ 
               httpfault
               );
      }
  
    delete http_response;  
    return status;
  }

  void LutsDestination::clear()
  {
    urn=0;
    joblogs.clear();
    usagerecordset.Replace(
        Arc::XMLNode(Arc::NS("",
                             "http://schema.ogf.org/urf/2003/09/urf"
                            ),
                     "UsageRecords")
                    );
  }

  LutsDestination::~LutsDestination()
  {
    finish();
  }
}
