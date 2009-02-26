#include "LUTSClient.h"

namespace Arc
{

  Arc::MCC_Status LUTSClient::log_urs(const std::string &urset)
  {
    Arc::NS _empty_ns;
    Arc::PayloadSOAP req(_empty_ns);
    Arc::PayloadSOAP *resp=NULL;
    Arc::Message inmsg,outmsg;

    //Create MCC loader. This also sets up TCP connection!
    mccloader=new Arc::MCCLoader(config);
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
