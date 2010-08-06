// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url, const UserConfig& usercfg)
    : url(url),
      usercfg(usercfg),
      size((unsigned long long int)(-1)),
      created(-1),
      valid(-1),
      access_latency(ACCESS_LATENCY_ZERO),
      triesleft(1),
      failure_code(DataStatus::UnknownError),
      cache(url.Option("cache") != "no") {
    // add standard options applicable to all protocols
    valid_url_options.clear();
    valid_url_options.push_back("cache");
    valid_url_options.push_back("readonly");
    valid_url_options.push_back("blocksize");
    valid_url_options.push_back("checksum");
    valid_url_options.push_back("exec");
    valid_url_options.push_back("preserve");
    valid_url_options.push_back("overwrite");
    valid_url_options.push_back("threads");
    valid_url_options.push_back("secure");
    valid_url_options.push_back("autodir");
  }

  DataPoint::~DataPoint() {}

  const URL& DataPoint::GetURL() const {
    return url;
  }

  const UserConfig& DataPoint::GetUserConfig() const {
    return usercfg;
  }

  std::string DataPoint::str() const {
    return url.str();
  }

  DataPoint::operator bool() const {

    if (!url)
      return false;
      
    // URL option validation. Subclasses which do not want to validate
    // URL options should override this method.
    std::map<std::string, std::string> options = url.Options();
    for (std::map<std::string, std::string>::iterator i = options.begin(); i != options.end(); i++) {
      bool valid = false; 
      for (std::list<std::string>::const_iterator j = valid_url_options.begin(); j != valid_url_options.end(); j++) {
        if (i->first == *j) valid = true;
      }
      if (!valid) {
        logger.msg(ERROR, "Invalid URL option: %s", i->first);
        return false;
      }
    }
    return true;
  }

  bool DataPoint::operator!() const {
    return !((bool)*this);
  }

  DataStatus DataPoint::GetFailureReason() const {
    return failure_code;
  }
  
  bool DataPoint::Cache() const {
    return cache;
  }
  
  bool DataPoint::CheckSize() const {
    return (size != (unsigned long long int)(-1));
  }

  void DataPoint::SetSize(const unsigned long long int val) {
    size = val;
  }

  unsigned long long int DataPoint::GetSize() const {
    return size;
  }

  bool DataPoint::CheckCheckSum() const {
    return (!checksum.empty());
  }

  void DataPoint::SetCheckSum(const std::string& val) {
    checksum = val;
  }

  const std::string& DataPoint::GetCheckSum() const {
    return checksum;
  }
    
  const std::string DataPoint::DefaultCheckSum() const {
    return std::string("cksum");
  }

  bool DataPoint::CheckCreated() const {
    return (created != -1);
  }

  void DataPoint::SetCreated(const Time& val) {
    created = val;
  }

  const Time& DataPoint::GetCreated() const {
    return created;
  }

  bool DataPoint::CheckValid() const {
    return (valid != -1);
  }

  void DataPoint::SetValid(const Time& val) {
    valid = val;
  }

  const Time& DataPoint::GetValid() const {
    return valid;
  }

  void DataPoint::SetAccessLatency(const DataPointAccessLatency& val) {
    access_latency = val;
  }

  DataPoint::DataPointAccessLatency DataPoint::GetAccessLatency() const {
    return access_latency;
  }

  int DataPoint::GetTries() const {
    return triesleft;
  }

  void DataPoint::SetTries(const int n) {
    triesleft = std::max(0, n);
  }

  void DataPoint::NextTry() {
    if(triesleft) --triesleft;
  }

  void DataPoint::SetMeta(const DataPoint& p) {
    if (!CheckSize())
      SetSize(p.GetSize());
    if (!CheckCheckSum())
      SetCheckSum(p.GetCheckSum());
    if (!CheckCreated())
      SetCreated(p.GetCreated());
    if (!CheckValid())
      SetValid(p.GetValid());
  }

  bool DataPoint::CompareMeta(const DataPoint& p) const {
    if (CheckSize() && p.CheckSize())
      if (GetSize() != p.GetSize())
        return false;
    if (CheckCheckSum() && p.CheckCheckSum())
      // TODO: compare checksums properly
      if (strcasecmp(GetCheckSum().c_str(), p.GetCheckSum().c_str()))
        return false;
    if (CheckValid() && p.CheckValid())
      if (GetValid() != p.GetValid())
        return false;
    return true;
  }

  DataPointLoader::DataPointLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  DataPointLoader::~DataPointLoader() {}

  DataPoint* DataPointLoader::load(const URL& url, const UserConfig& usercfg) {
    DataPointPluginArgument arg(url, usercfg);
    factory_->load(FinderLoader::GetLibrariesList(), "HED:DMC");
    return factory_->GetInstance<DataPoint>("HED:DMC", &arg, false);
  }

  DataPointLoader DataHandle::loader;

} // namespace Arc
