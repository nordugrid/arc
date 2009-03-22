#include "LUTSClient.h"
#include <sstream>

namespace Arc
{
  
  LUTSClient::LUTSClient(Arc::Config& cfg): clientchain(Arc::NS())
  /**
   * Constructor. Sets up client chain based on given configuration XML.
   */
  {
    std::string dump;

    Arc::Logger logger(Arc::Logger::rootLogger, "Lutsclient.LUTSClient");

    Arc::NS ns;
    ns[""]="http://www.nordugrid.org/schemas/ArcConfig/2007";
    ns["tcp"]="http://www.nordugrid.org/schemas/ArcMCCTCP/2007";
    clientchain.Namespaces(ns);

    //Get service URL, cert, key, CA path from server (or separate) config

    std::string serviceurl, certfile, keyfile, cadir;
    Arc::XMLNode confvalue;

    //  By default, use the same cert, key, CA path as the service
    Arc::XMLNode component=cfg["Chain"]["Component"];
    while (component && 
	   (std::string)component.Attribute("name") != "tls.service" 
	   )
      {
	++component;
      }
    if (confvalue=component["CertificatePath"])
      certfile=(std::string)confvalue;
    if (confvalue=component["KeyPath"])
      keyfile=(std::string)confvalue;
    if (confvalue=component["CACertificatesDir"])
      cadir=(std::string)confvalue;

    //  Values under "LutsClient" override defaults
    Arc::XMLNode lutsclientnode=cfg["LutsClient"];
    if (confvalue=lutsclientnode["ServiceURL"])
      serviceurl=(std::string)confvalue;
    if (confvalue=lutsclientnode["CertificatePath"])
      certfile=(std::string)confvalue;
    if (confvalue=lutsclientnode["KeyPath"])
      keyfile=(std::string)confvalue;
    if (confvalue=lutsclientnode["CACertificatesDir"])
      cadir=(std::string)confvalue;

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

    //MCC module path(s):
    Arc::XMLNode modulemanager=cfg["ModuleManager"];
    if (modulemanager)
      {
	clientchain.NewChild("ModuleManager").Replace(modulemanager);
      }

    //The protocol stack: SOAP over HTTP over SSL over TCP
    clientchain.NewChild("Plugins").NewChild("Name")="mcctcp";
    clientchain.NewChild("Plugins").NewChild("Name")="mcctls";
    clientchain.NewChild("Plugins").NewChild("Name")="mcchttp";
    clientchain.NewChild("Plugins").NewChild("Name")="mccsoap";


    //The chain
    Arc::XMLNode chain=clientchain.NewChild("Chain");
  
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

  }

  Arc::MCC_Status LUTSClient::log_urs(const std::string &urset)
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
}
