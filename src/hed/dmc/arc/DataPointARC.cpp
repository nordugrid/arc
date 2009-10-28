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
#include <glibmm.h>
#include <algorithm>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>

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

  bool DataPointARC::checkBartenderURL(const URL& bartender_url){
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;

    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:list").NewChild("bar:listRequestList").NewChild("bar:listRequestElement").NewChild("bar:requestID") = "0";
    request["bar:list"]["bar:listRequestList"]["bar:listRequestElement"].NewChild("bar:LN") = bartender_url.Path();
    request["bar:list"].NewChild("bar:neededMetadataList").NewChild("bar:neededMetadataElement").NewChild("bar:section") = "entry";
    request["bar:list"]["bar:neededMetadatList"]["bar:neededMetadataElement"].NewChild("bar:property") = "";
    request.GetXML(xml, true);

    PayloadSOAP *response = NULL;

    MCC_Status status;
    try{
      status = client.process(&request, &response);
    }
    catch(std::exception e){
      logger.msg(WARNING, bartender_url.Path(), "did not work");
    }

    bool ret = true;
    if(!response)
      ret = false;
    else{
      response->Child().GetXML(xml, true);
      logger.msg(DEBUG, "checingBartenderURL: Response:\n%s", xml);
      if(xml.find("Failed to send SOAP message") != std::string::npos)
        ret = false;
    }
    if(!status)
      ret = false;
    delete response;
    return ret;
  }

  DataPointARC::DataPointARC(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      transfer(NULL),
      md5sum(NULL),
      reading(false),
      writing(false),
      bartender_url(url.HTTPOption("BartenderURL")) {
    if (!bartender_url) {
      if (!usercfg.Bartender().empty()){
        std::vector<int> shuffledKeys;
        for (int i = 0; i < usercfg.Bartender().size(); i++)
          shuffledKeys.push_back(i);
        std::random_shuffle(shuffledKeys.begin(), shuffledKeys.end());

        // pick random bartender url:
        for (int i = 0; i < shuffledKeys.size(); i++) {
          if (checkBartenderURL(usercfg.Bartender()[shuffledKeys[i]])) {
            bartender_url = usercfg.Bartender()[shuffledKeys[i]];
            break;
          }
        }
      }
      if (!bartender_url)
        bartender_url = URL("http://localhost:60000/Bartender");
    }

    md5sum = new MD5Sum();
  }

  DataPointARC::~DataPointARC() {
    StopReading();
    StopWriting();
    if (md5sum) {
      delete md5sum;
      md5sum = NULL;
    }
    if (transfer){
      delete transfer;
      transfer = NULL;
    }
  }

  Plugin* DataPointARC::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "arc")
      return NULL;
    return new DataPointARC(*dmcarg, *dmcarg);
  }

  DataStatus DataPointARC::ListFiles(std::list<FileInfo>& files, bool, bool, bool) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;

    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:list").NewChild("bar:listRequestList").NewChild("bar:listRequestElement").NewChild("bar:requestID") = "0";
    request["bar:list"]["bar:listRequestList"]["bar:listRequestElement"].NewChild("bar:LN") = url.Path();
    request["bar:list"].NewChild("bar:neededMetadataList").NewChild("bar:neededMetadataElement").NewChild("bar:section") = "entry";
    request["bar:list"]["bar:neededMetadatList"]["bar:neededMetadataElement"].NewChild("bar:property") = "";
    request.GetXML(xml, true);
    logger.msg(INFO, "Request:\n%s", xml);

    PayloadSOAP *response = NULL;

    MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::ListError;
    }

    if (!response) {
      logger.msg(ERROR, "No SOAP response");
      return DataStatus::ListError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["listResponseList"]["listResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(INFO, "nd:\n%s", xml);

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
        logger.msg(INFO, "cnd:\n%s is a %s", file_name, type);
        std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(file_name.c_str()));
        if (type == "collection")
          f->SetType(FileInfo::file_type_dir);
        else
          f->SetType(FileInfo::file_type_file);
      }
    else {
      // its a file or something
      // we know it exists so we use file name from url
      std::string path = url.Path();
      std::string::size_type i = path.rfind(G_DIR_SEPARATOR, path.length());
      std::string file_name = path.substr((i != std::string::npos)?(i+1):0);
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(file_name.c_str()));
      f->SetType(FileInfo::file_type_file);
    }

    std::string answer = (std::string)((*response).Child().Name());

    delete response;

    logger.msg(INFO, answer);

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
    usercfg.ApplyToConfig(cfg);

    // get TURL from bartender
    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;

    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:getFile").NewChild("bar:getFileRequestList").NewChild("bar:getFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:getFile"]["bar:getFileRequestList"]["bar:getFileRequestElement"].NewChild("bar:LN") = url.Path();
    // only supports http protocol:
    request["bar:getFile"]["bar:getFileRequestList"]["bar:getFileRequestElement"].NewChild("bar:protocol") = "http";
    request.GetXML(xml, true);
    logger.msg(INFO, "Request:\n%s", xml);

    PayloadSOAP *response = NULL;

    MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::ReadError;
    }

    if (!response) {
      logger.msg(ERROR, "No SOAP response");
      return DataStatus::ReadError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["getFileResponseList"]["getFileResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(INFO, "nd:\n%s", xml);

    if (nd["success"] != "done" || !nd["TURL"]) {
      delete response;
      return DataStatus::ReadError;
    }

    logger.msg(INFO, "Recieved transfer URL: %s", (std::string)nd["TURL"]);

    turl = (std::string) nd["TURL"];
    delete response;
    delete response;
    // redirect actual reading to http dmc
    if (transfer){ 
      delete transfer;
      transfer = NULL;
    }
    transfer = new DataHandle(turl, usercfg);
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
    chksum_index = buffer->add(md5sum);
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    // get TURL from bartender
    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;
    std::stringstream out;
    out << this->GetSize();
    std::string size_str = out.str();
    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:putFile").NewChild("bar:putFileRequestList").NewChild("bar:putFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:LN") = url.Path();
    // only supports http protocol:
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:protocol") = "http";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"].NewChild("bar:metadataList").NewChild("bar:metadata").NewChild("bar:section") = "states";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"].NewChild("bar:property") = "checksum";
    request["bar:putFile"]["bar:putFileRequestList"]["bar:putFileRequestElement"]["bar:metadataList"]["bar:metadata"].NewChild("bar:value") = "";
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
    logger.msg(INFO, "Request:\n%s", xml);

    PayloadSOAP *response = NULL;

    MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::WriteError;
    }

    if (!response) {
      logger.msg(ERROR, "No SOAP response");
      return DataStatus::WriteError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["putFileResponseList"]["putFileResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(INFO, "nd:\n%s", xml);

    if (nd["success"] != "done" || !nd["TURL"]) {
      delete response;
      return DataStatus::WriteError;
    }

    logger.msg(INFO, "Recieved transfer URL: %s", (std::string)nd["TURL"]);

    turl = (std::string) nd["TURL"];
    // redirect actual writing to http dmc
    if (transfer){
      delete transfer;
      transfer = NULL;
    }
    transfer = new DataHandle(turl, usercfg);
    delete response;
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
    unsigned char *md5res_u = new unsigned char(16);
    unsigned int length;
    md5sum->result(md5res_u, length);
    std::string md5str = "";
    for (int i = 0; i < length; i++) {
      char tmpChar[3];
      sprintf(tmpChar, "%.2x", md5res_u[i]);
      md5str += tmpChar;
    }
    // TODO: figure out how to delete md5res_u
    // This causes segfaults:
    // delete [] md5res_u;
    logger.msg(DEBUG, "Calculated checksum: %s", md5str);

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    // get TURL from bartender
    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;
    std::stringstream out;
    out << this->GetSize();
    std::string size_str = out.str();
    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:modify").NewChild("bar:modifyRequestList").NewChild("bar:modifyRequestElement").NewChild("bar:changeID") = "0";
    request["bar:modify"]["bar:modifyRequestList"]["bar:modifyRequestElement"].NewChild("bar:LN") = url.Path();
    request["bar:modify"]["bar:modifyRequestList"]["bar:modifyRequestElement"].NewChild("bar:changeType") = "set";
    request["bar:modify"]["bar:modifyRequestList"]["bar:modifyRequestElement"].NewChild("bar:section") = "states";
    request["bar:modify"]["bar:modifyRequestList"]["bar:modifyRequestElement"].NewChild("bar:property") = "checksum";
    request["bar:modify"]["bar:modifyRequestList"]["bar:modifyRequestElement"].NewChild("bar:value") = md5str;
    request.GetXML(xml, true);
    logger.msg(INFO, "Request:\n%s", xml);

    PayloadSOAP *response = NULL;

    MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::WriteError;
    }

    if (!response) {
      logger.msg(ERROR, "No SOAP response");
      return DataStatus::WriteError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["modifyResponseList"]["modifyResponseElement"];
    nd.GetXML(xml, true);

    logger.msg(INFO, "nd:\n%s", xml);

    if (nd["success"] != "set")
      return DataStatus::WriteError;

    delete md5sum;
    md5sum = NULL;
    delete transfer;
    transfer = NULL;
    return ret;
  }

  DataStatus DataPointARC::Check() {
    return DataStatus::Success;
  }

  DataStatus DataPointARC::Remove() {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    ClientSOAP client(cfg, bartender_url, usercfg.Timeout());
    std::string xml;

    NS ns("bar", "http://www.nordugrid.org/schemas/bartender");
    PayloadSOAP request(ns);
    request.NewChild("bar:delFile").NewChild("bar:delFileRequestList").NewChild("bar:delFileRequestElement").NewChild("bar:requestID") = "0";
    request["bar:delFile"]["bar:delFileRequestList"]["bar:delFileRequestElement"].NewChild("bar:LN") = url.Path();

    request.GetXML(xml, true);
    logger.msg(INFO, "Request:\n%s", xml);

    PayloadSOAP *response = NULL;

    MCC_Status status = client.process(&request, &response);

    if (!status) {
      logger.msg(ERROR, (std::string)status);
      if (response)
        delete response;
      return DataStatus::DeleteError;
    }

    if (!response) {
      logger.msg(ERROR, "No SOAP response");
      return DataStatus::DeleteError;
    }

    response->Child().GetXML(xml, true);
    logger.msg(INFO, "Response:\n%s", xml);

    XMLNode nd = (*response).Child()["delFileResponseList"]["delFileResponseElement"];

    if (nd["success"] == "deleted")
      logger.msg(INFO, "Deleted %s", url.Path());
    delete response;
    return DataStatus::Success;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "arc", "HED:DMC", 0, &Arc::DataPointARC::Instance },
  { NULL, NULL, 0, NULL }
};
