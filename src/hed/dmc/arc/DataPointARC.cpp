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
      url.ChangeProtocol("http");
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
