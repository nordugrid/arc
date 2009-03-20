// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_LIMIT_MACROS
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <sstream>

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
      writing(false),
      usercfg(""),
      bartender_url("") {
    //BartenderURL taken from ~/.arc/client.xml
    std::string bartender_str = (std::string)usercfg.ConfTree()["BartenderURL"];
    //todo: improve default bartender url (maybe try to get ARC_BARTENDER_URL from environment?)
    if (bartender_str.empty())
      bartender_str = "http://localhost:60000/Bartender";
    //URL bartender_url(url.ConnectionURL()+"/Bartender");
    bartender_url = URL(bartender_str);
    md5sum = new MD5Sum();
  }

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

    Arc::ClientSOAP client(cfg, bartender_url);
    std::string xml;

    Arc::NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    Arc::PayloadSOAP request(ns);
    request.NewChild("bar:list").NewChild("bar:listRequestList").NewChild("bar:listRequestElement").NewChild("bar:requestID") = "0";
    request["bar:list"]["bar:listRequestList"]["bar:listRequestElement"].NewChild("bar:LN") = "/" + url.Path();
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

    XMLNode nd = (*response).Child()["listResponseList"]["listResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(Arc::INFO, "nd:\n%s", xml);

    if (nd["status"] == "not found")
      return DataStatus::ListError;

    if (nd["status"] == "found")
      for (int i = 0;; i++) {
        XMLNode cnd = nd["entries"]["entry"][i];
        if (!cnd)
          break;
        std::string file_name = cnd["name"];
        std::string type;
        for (int j = 0;; j++) {
          XMLNode ccnd = cnd["metadataList"]["metadata"][j];
          if (!ccnd)
            break;
          if (ccnd["property"] == "type")
            type = (std::string)ccnd["value"];
        }
        logger.msg(Arc::INFO, "cnd:\n%s is a %s", file_name, type);
        std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(file_name.c_str()));
        if (type == "collection")
          f->SetType(FileInfo::file_type_dir);
        else
          f->SetType(FileInfo::file_type_file);
      }
    else {
      // its a file or something
      // we know it exists so we use file name from url
      char sep = '/';

      #ifdef _WIN32
      sep = '\\';
      #endif
      std::string path = url.Path();
      size_t i = path.rfind(sep, path.length());
      std::string file_name = path.substr(i + 1, path.length() - i);
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(file_name.c_str()));
      f->SetType(FileInfo::file_type_file);
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

    // get TURL from bartender
    Arc::ClientSOAP client(cfg, bartender_url);
    std::string xml;

    Arc::NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    Arc::PayloadSOAP request(ns);
    request.NewChild("bar:getFile").NewChild("bar:getFileRequestList").NewChild("bar:getFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:getFile"]["bar:getFileRequestList"]["bar:getFileRequestElement"].NewChild("bar:LN") = "/" + url.Path();
    // only supports http protocol:
    request["bar:getFile"]["bar:getFileRequestList"]["bar:getFileRequestElement"].NewChild("bar:protocol") = "http";
    request.GetXML(xml, true);
    logger.msg(Arc::INFO, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(Arc::ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::ReadError;
    }

    if (!response) {
      logger.msg(Arc::ERROR, "No SOAP response");
      return DataStatus::ReadError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(Arc::INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["getFileResponseList"]["getFileResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(Arc::INFO, "nd:\n%s", xml);

    if (nd["success"] != "done" || !nd["TURL"])
      return DataStatus::ReadError;

    logger.msg(Arc::INFO, "Recieved transfer URL: %s", (std::string)nd["TURL"]);

    URL turl(nd["TURL"]);
    // redirect actual reading to http dmc
    transfer = new DataHandle(turl);
    (*transfer)->AssignCredentials(proxyPath,
                                   certificatePath,
                                   keyPath,
                                   caCertificatesDir);
    if (!(*transfer)->StartReading(buf)) {
      if (transfer) {
        delete transfer;
        transfer = NULL;
      }
      reading = false;
      return DataStatus::ReadError;
    }

    return DataStatus::Success;
  }

  DataStatus DataPointARC::StopReading() {
    if (!reading)
      return DataStatus::ReadStopError;
    reading = false;
    if (!transfer)
      return DataStatus::Success;
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
    buffer->set(md5sum);
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);

    // get TURL from bartender
    Arc::ClientSOAP client(cfg, bartender_url);
    std::string xml;
    std::stringstream out;
    out << this->GetSize();
    std::string size_str = out.str();
    Arc::NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    Arc::PayloadSOAP request(ns);
    request.NewChild("bar:putFile").NewChild("bar:putFileRequestList").NewChild("bar:putFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:LN") = "/" + url.Path();
    // only supports http protocol:
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:protocol") = "http";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:metadataList").NewChild("bar:metadata").NewChild("bar:section") = "states";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"].NewChild("bar:property") = "checksum";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"].NewChild("bar:value") = this->GetCheckSum();
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"].NewChild("bar:metadata").NewChild("bar:section") = "states";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][1].NewChild("bar:property") = "checksumType";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][1].NewChild("bar:value") = "md5";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"].NewChild("bar:metadata").NewChild("bar:section") = "states";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][2].NewChild("bar:property") = "neededReplicas";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][2].NewChild("bar:value") = "1";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"].NewChild("bar:metadata").NewChild("bar:section") = "states";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][3].NewChild("bar:property") = "size";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"][3].NewChild("bar:value") = size_str;
    request.GetXML(xml, true);
    logger.msg(Arc::INFO, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(Arc::ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::WriteError;
    }

    if (!response) {
      logger.msg(Arc::ERROR, "No SOAP response");
      return DataStatus::WriteError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(Arc::INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["putFileResponseList"]["putFileResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(Arc::INFO, "nd:\n%s", xml);

    if (nd["success"] != "done" || !nd["TURL"])
      return DataStatus::WriteError;

    logger.msg(Arc::INFO, "Recieved transfer URL: %s", (std::string)nd["TURL"]);

    URL turl(nd["TURL"]);
    // redirect actual writing to http dmc
    transfer = new DataHandle(turl);
    (*transfer)->AssignCredentials(proxyPath,
                                   certificatePath,
                                   keyPath,
                                   caCertificatesDir);
    if (!(*transfer)->StartWriting(buf, callback)) {
      if (transfer) {
        delete transfer;
        transfer = NULL;
      }
      writing = false;
      return DataStatus::WriteError;
    }
    return DataStatus::Success;

  }

  DataStatus DataPointARC::StopWriting() {
    if (!writing)
      return DataStatus::WriteStopError;
    writing = false;
    if (!transfer)
      return DataStatus::Success;
    // update checksum and size
    DataStatus ret = (*transfer)->StopWriting();
    buffer->wait_read();
    unsigned char *md5res = new unsigned char(16);
    unsigned int length = 0;
    //md5sum->end();
    md5sum->result(md5res, length);
    std::string md5str = "";
    for (int i = 0; i < length; i++) {
      char tmpChar[2];
      sprintf(tmpChar, "%.2x", md5res[i]);
      md5str += tmpChar;
    }
    std::cout << "CheckSum: " << md5str << " number " << length << " valid " << (buffer->checksum_valid() ? "yes":"no") << std::endl;
    logger.msg(Arc::INFO, "Checksum? %s", md5str);
    delete transfer;
    delete md5sum;
    transfer = NULL;
    return ret;
  }

  DataStatus DataPointARC::Check() {
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

    Arc::ClientSOAP client(cfg, bartender_url);
    std::string xml;

    Arc::NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    Arc::PayloadSOAP request(ns);
    request.NewChild("bar:delFile").NewChild("bar:delFileRequestList").NewChild("bar:delFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:delFile"]["bar:delFileRequestList"]["bar:delFileRequestElement"].NewChild("bar:LN") = "/" + url.Path();

    request.GetXML(xml, true);
    logger.msg(Arc::INFO, "Request:\n%s", xml);

    Arc::PayloadSOAP *response = NULL;

    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(Arc::ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::DeleteError;
    }

    if (!response) {
      logger.msg(Arc::ERROR, "No SOAP response");
      return DataStatus::DeleteError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(Arc::INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["delFileResponseList"]["delFileResponseElement"];

    if (nd["success"] == "deleted")
      logger.msg(Arc::INFO, "Deleted %s", url.Path());
    return DataStatus::Success;
  }

} // namespace Arc
