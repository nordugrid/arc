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
#include <arc/data/DataHandle.h>


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
    : DataPointDirect(url) {}

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

  DataStatus DataPointARC::StartReading(DataBuffer& buffer) {
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
	url.ChangeProtocol("http");
	// redirect actual reading to http dmc
	logger.msg(Arc::ERROR, "URL is now %s", url.str());
	Arc::DataHandle transfer(url);
	return transfer->StartReading(buffer);
  }

  DataStatus DataPointARC::StopReading() {
    return DataStatus::Success;
  }

  DataStatus DataPointARC::StartWriting(DataBuffer& buffer,
                                         DataCallback *) {
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

  DataStatus DataPointARC::StopWriting() {
    return DataStatus::Success;
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
