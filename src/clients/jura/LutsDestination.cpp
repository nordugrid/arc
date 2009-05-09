#include "LutsDestination.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"
#include "arc/Utils.h"
#include <sstream>

namespace Arc
{
  LutsDestination::LutsDestination(JobLogFile& joblog):
    logger(Arc::Logger::rootLogger, "JURA.LutsDestination"),
    urn(0),
    usagerecordset(Arc::NS("","http://schema.ogf.org/urf/2003/09/urf"),
                   "UsageRecords")
  {
    std::string dump;

    //Set up the Client Message Chain:
    Arc::NS ns;
    ns[""]="http://www.nordugrid.org/schemas/ArcConfig/2007";
    ns["tcp"]="http://www.nordugrid.org/schemas/ArcMCCTCP/2007";
    clientchain.Namespaces(ns);

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

#ifdef HAVE_CONFIG_H
    //MCC module path(s):
    clientchain.NewChild("ModuleManager").NewChild("Path")=
      INSTPREFIX "/" LIBSUBDIR;
    clientchain["ModuleManager"].NewChild("Path")=
      INSTPREFIX "/" PKGLIBSUBDIR;
#endif

    //The protocol stack: SOAP over HTTP over SSL over TCP
    clientchain.NewChild("Plugins").NewChild("Name")="mcctcp";
    clientchain.NewChild("Plugins").NewChild("Name")="mcctls";
    clientchain.NewChild("Plugins").NewChild("Name")="mcchttp";
    clientchain.NewChild("Plugins").NewChild("Name")="mccsoap";


    //The chain
    Arc::XMLNode chain=clientchain.NewChild("Chain");
    Arc::XMLNode component;
  
    //  TCP
    component=chain.NewChild("Component");
    component.NewAttribute("name")="tcp.client";
    component.NewAttribute("id")="tcp";
    Arc::XMLNode connect=component.NewChild("tcp:Connect");
    connect.NewChild("tcp:Host")=host;
    connect.NewChild("tcp:Port")=port;

    //  TLS (SSL)
    component=chain.NewChild("Component");
    component.NewAttribute("name")="tls.client";
    component.NewAttribute("id")="tls";
    component.NewChild("next").NewAttribute("id")="tcp";
    if (!certfile.empty())
      component.NewChild("CertificatePath")=certfile;
    if (!keyfile.empty())
      component.NewChild("KeyPath")=keyfile;
    if (!cadir.empty())
      component.NewChild("CACertificatesDir")=cadir;
  
    //  HTTP
    component=chain.NewChild("Component");
    component.NewAttribute("name")="http.client";
    component.NewAttribute("id")="http";
    component.NewChild("next").NewAttribute("id")="tls";
    component.NewChild("Method")="POST";
    component.NewChild("Endpoint")=std::string("/")+endpoint;
  
    //  SOAP
    component=chain.NewChild("Component");
    component.NewAttribute("name")="soap.client";
    component.NewAttribute("id")="soap";
    component.NewAttribute("entry")="soap";
    component.NewChild("next").NewAttribute("id")="http";

    clientchain.GetDoc(dump,true);
    logger.msg(Arc::DEBUG, "Client chain configuration: %s",
               dump.c_str() );

    //Get Batch Size:
    //TODO get from jobreport_options!!!
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
    if (joblog.exists())
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
    logger.msg(Arc::VERBOSE, 
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
    Arc::NS _empty_ns;
    Arc::PayloadSOAP req(_empty_ns);
    Arc::PayloadSOAP *resp=NULL;
    Arc::Message inmsg,outmsg;

    //Create MCC loader. This also sets up TCP connection!
    mccloader=new Arc::MCCLoader(clientchain);
    //(and we also get a load of log entries)

    soapmcc=(*mccloader)["soap"];
    if (!soapmcc)
    {
      delete mccloader;
      return Arc::MCC_Status(Arc::GENERIC_ERROR,
                             "lutsclient",
                             "No SOAP entry point in chain");
    }

    //TODO ws-addressing!

    //Build request structure:
    Arc::NS ns_wsrp("",
     "http://docs.oasis-open.org/wsrf/2004/06/wsrf-WS-ResourceProperties-1.2-draft-01.xsd"
                    );
    Arc::XMLNode query=req.NewChild("QueryResourceProperties",ns_wsrp).
                           NewChild("QueryExpression");
    query.NewAttribute("Dialect")=
      "http://www.sgas.se/namespaces/2005/06/publish/query";
    query=urset;
    //put into message:
    inmsg.Payload(&req);

    //Send
    Arc::MCC_Status status;

    status=soapmcc->process(inmsg, outmsg);

    //extract response:
    try
    {
      resp=dynamic_cast<Arc::PayloadSOAP*>(outmsg.Payload());
    }
    catch (std::exception&) {}

    if (resp==NULL)
      {
        //Unintelligible non-SOAP response
        delete mccloader;
        return Arc::MCC_Status(Arc::PROTOCOL_RECOGNIZED_ERROR,
                               "lutsclient",
                               "Response not SOAP");
      }

    if (status && ! ((*resp)["QueryResourcePropertiesResponse"]))
      {
        // Status OK, but wrong response
        std::string soapfault;
        resp->GetDoc(soapfault,false);

        delete mccloader;
        return Arc::MCC_Status(Arc::PARSING_ERROR,
               "lutsclient",
               std::string(
                 "No QueryResourcePropertiesResponse element in response: "
                           )+ 
               soapfault
               );
      }
    
    delete mccloader;
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
