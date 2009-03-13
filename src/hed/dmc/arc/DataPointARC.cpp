#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_LIMIT_MACROS
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>

#include <arc/Logger.h>
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/client/ClientInterface.h>


#ifdef WIN32
#include <arc/win32.h>
#endif

#include "DataPointARC.h"

namespace Arc {

  Logger DataPointARC::logger(DataPoint::logger, "ARC");

  typedef struct {
    DataPointARC *point;
    ClientSOAP *client;
  } ARCInfo_t;


  DataPointARC::DataPointARC(const URL& url)
    : DataPointDirect(url),
      transfer(NULL),
      reading(false),
      writing(false) {}

  DataPointARC::~DataPointARC() {
    StopReading();
    StopWriting();
  }

  DataStatus DataPointARC::ListFiles(std::list<FileInfo>& files, bool, bool) {
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);

	if(url.Protocol()=="arc"){
      url.ChangeProtocol("https");
    }
	URL bartender_url(url.ConnectionURL()+"/Bartender");

    Arc::ClientSOAP client(cfg, bartender_url);
    std::string xml;
	
    Arc::NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    Arc::PayloadSOAP request(ns);
    request.NewChild("bar:list").NewChild("bar:listRequestList").NewChild("bar:listRequestElement").NewChild("bar:requestID") = "0";
    request["bar:list"]["bar:listRequestList"]["bar:listRequestElement"].NewChild("bar:LN") = "/"+url.Path();
	request["bar:list"].NewChild("bar:neededMetadataList").NewChild("bar:neededMetadataElement").NewChild("bar:section") = "entry";
	request["bar:list"]["bar:neededMetadatList"]["bar:neededMetadataElement"].NewChild("bar:property") = "";
    request.GetXML(xml, true);
    logger.msg(Arc::INFO, "Request:\n%s", xml);
	
    Arc::PayloadSOAP *response = NULL;
	
    Arc::MCC_Status status = client.process(&request, &response);
	
    if (!status) {
      logger.msg(Arc::ERROR, (std::string)status);
      if (response)
        delete response;
        return DataStatus::ListError;
    }
	
    if (!response) {
      logger.msg(Arc::ERROR, "No SOAP response");
      return DataStatus::ListError;
    }
	
    response->Child().GetXML(xml, true);
    logger.msg(Arc::INFO, "Response:\n%s", xml);
	
	XMLNode nd = (*response).Child()["listResponseList"]["listResponseElement"]["entries"]["entry"];
	nd.GetXML(xml,true);
	logger.msg(Arc::INFO, "nd:\n%s", xml); 
	
	for(int i=0;;i++){
	  XMLNode cnd = nd[i];
	  if(!cnd) break;
      std::string name = cnd["name"];
      std::string type;
      for(int j=0;;j++){
        XMLNode ccnd = cnd["metadataList"]["metadata"][j];
        if(!ccnd) break;
	    if(ccnd["property"] == "type") type = (std::string) ccnd["value"];
      }
	  logger.msg(Arc::INFO, "cnd:\n%s is a %s", name, type);  
	}
		
    std::string answer = (std::string)((*response).Child().Name());
	
    delete response;
	
   	logger.msg(Arc::INFO, answer);

    return DataStatus::Success;
  }

  DataStatus DataPointARC::StartReading(DataBuffer& buf) {
    logger.msg(DEBUG, "StartReading");
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
	
	reading = true;
	buffer = &buf;
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
	logger.msg(Arc::ERROR, "Received URL with protocol %s", url.Protocol());
	if(url.Protocol()=="arc"){
      url.ChangeProtocol("https");
    }
	// redirect actual reading to http dmc
	logger.msg(Arc::ERROR, "URL is now %s", url.str());
	transfer = new DataHandle(url);
	if(!(*transfer)->StartReading(buf)){
      if(transfer) { delete transfer; transfer = NULL; };
      reading = false;
      return DataStatus::ReadError;
    }

  	return DataStatus::Success;
  }

  DataStatus DataPointARC::StopReading() {
	if (!reading)
      return DataStatus::ReadStopError;
    reading = false;
    if(!transfer) return DataStatus::Success;
	DataStatus ret = (*transfer)->StopReading();	
	delete transfer;
	transfer = NULL;
    return ret;
  }

  DataStatus DataPointARC::StartWriting(DataBuffer& buf,
                                         DataCallback *callback) {
    logger.msg(DEBUG, "StartWriting");
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;

    writing = true;
    buffer = &buf;

    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    return DataStatus::Success;

	if(!(*transfer)->StartWriting(buf, callback)){
      if(transfer) { delete transfer; transfer = NULL; };
      writing = false;
      return DataStatus::WriteError;	  
	}
  }

  DataStatus DataPointARC::StopWriting() {
	if (!writing)
      return DataStatus::WriteStopError;
    writing = false;
    if(!transfer) return DataStatus::Success;
	DataStatus ret = (*transfer)->StopWriting();	
	delete transfer;
	transfer = NULL;
    return ret;
  }

  DataStatus DataPointARC::Check() {
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    return DataStatus::Success;
  }

  DataStatus DataPointARC::Remove() {
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    return DataStatus::DeleteError;
  }

} // namespace Arc
