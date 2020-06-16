// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : Plugin(parg),
      url(url),
      usercfg(usercfg),
      size((unsigned long long int)(-1)),
      modified(-1),
      valid(-1),
      access_latency(ACCESS_LATENCY_ZERO),
      triesleft(1),
      failure_code(DataStatus::UnknownError),
      cache(url.Option("cache") != "no"),
      stageable(false) {
    // add all valid URL options applicable to all protocols
    valid_url_options.clear();
    valid_url_options.insert("cache");
    valid_url_options.insert("readonly");
    valid_url_options.insert("blocksize");
    valid_url_options.insert("checksum");
    valid_url_options.insert("exec");
    valid_url_options.insert("preserve");
    valid_url_options.insert("overwrite");
    valid_url_options.insert("threads");
    valid_url_options.insert("secure");
    valid_url_options.insert("autodir");
    valid_url_options.insert("tcpnodelay");
    valid_url_options.insert("protocol");
    valid_url_options.insert("spacetoken");
    valid_url_options.insert("transferprotocol");
    valid_url_options.insert("encryption");
    valid_url_options.insert("httpputpartial");
    valid_url_options.insert("httpgetpartial");
    valid_url_options.insert("rucioaccount");
    valid_url_options.insert("failureallowed");
    valid_url_options.insert("relativeuri");
  }

  DataPoint::~DataPoint() {}

  const URL& DataPoint::GetURL() const {
    return url;
  }

  bool DataPoint::SetURL(const URL& url) {
    return false;
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
      if (valid_url_options.find(i->first) == valid_url_options.end()) {
        logger.msg(ERROR, "Invalid URL option: %s", i->first);
        return false;
      }
    }
    return true;
  }

  bool DataPoint::operator!() const {
    return !((bool)*this);
  }

  DataStatus DataPoint::PrepareReading(unsigned int timeout,
                                       unsigned int& wait_time) {
    wait_time = 0;
    return DataStatus::Success;
  }

  DataStatus DataPoint::PrepareWriting(unsigned int timeout,
                                       unsigned int& wait_time) {
    wait_time = 0;
    return DataStatus::Success;
  }

  DataStatus DataPoint::FinishReading(bool error) {
    return DataStatus::Success;
  }

  DataStatus DataPoint::FinishWriting(bool error) {
    return DataStatus::Success;
  }

  DataStatus DataPoint::Transfer(const URL& otherendpoint, bool source, TransferCallback callback) {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  DataStatus DataPoint::Transfer3rdParty(const URL& source, const URL& destination, TransferCallback callback) {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  DataStatus DataPoint::GetFailureReason() const {
    return failure_code;
  }
  
  bool DataPoint::Cache() const {
    return cache;
  }
  
  bool DataPoint::IsStageable() const {
    return stageable;
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

  bool DataPoint::CheckModified() const {
    return (modified != -1);
  }

  void DataPoint::SetModified(const Time& val) {
    modified = val;
  }

  const Time& DataPoint::GetModified() const {
    return modified;
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

  bool DataPoint::RequiresCredentials() const {
    return true;
  }

  bool DataPoint::RequiresCredentialsInFile() const {
    return false;
  }

  bool DataPoint::SupportsTransfer() const {
    return false;
  }

  void DataPoint::SetMeta(const DataPoint& p) {
    if (!CheckSize())
      SetSize(p.GetSize());
    if (!CheckCheckSum())
      SetCheckSum(p.GetCheckSum());
    if (!CheckModified())
      SetModified(p.GetModified());
    if (!CheckValid())
      SetValid(p.GetValid());
  }

  void DataPoint::ResetMeta() {
    size = (unsigned long long int)(-1);
    checksum.clear();
    modified = -1;
    valid = -1;
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

  std::vector<URL> DataPoint::TransferLocations() const {
    // return empty vector
    std::vector<URL> urls;
    urls.push_back(url);
    return urls;
  }

  void DataPoint::AddURLOptions(const std::map<std::string, std::string>& options) {
    for (std::map<std::string, std::string>::const_iterator key = options.begin();
         key != options.end(); ++key) {

      if (valid_url_options.find(key->first) == valid_url_options.end()) {
        logger.msg(VERBOSE, "Skipping invalid URL option %s", key->first);
      } else {
        url.AddOption(key->first, key->second, true);
      }
    }
  }

  DataStatus DataPoint::Transfer3rdParty(const URL& source, const URL& destination,
                                         const UserConfig& usercfg, TransferCallback callback) {
    // to load GFAL instead of ARC's DMCs we create a fake URL with gfal protocol
    URL gfal_url(destination);
    gfal_url.ChangeProtocol("gfal");
    // load GFAL DMC
    DataHandle gfal_handle(gfal_url, usercfg);
    if (!gfal_handle) {
      logger.msg(Arc::ERROR, "Third party transfer was requested but the corresponding plugin could\n"
                      "       not be loaded. Is the GFAL plugin installed? If not, please install the\n"
                      "       packages 'nordugrid-arc-plugins-gfal' and 'gfal2-all'. Depending on\n"
                      "       your type of installation the package names might differ.");
      return DataStatus(DataStatus::TransferError, EOPNOTSUPP, "Could not load GFAL plugin");
    }
    return gfal_handle->Transfer3rdParty(source, destination, callback);
  }

  DataPointLoader::DataPointLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  DataPointLoader::~DataPointLoader() {}

  DataPoint* DataPointLoader::load(const URL& url, const UserConfig& usercfg) {
    DataPointPluginArgument arg(url, usercfg);
    factory_->load(FinderLoader::GetLibrariesList(), "HED:DMC");
    DataPoint* point = factory_->GetInstance<DataPoint>("HED:DMC", &arg, false);
    if(!point) logger.msg(Arc::VERBOSE, "Failed to load plugin for URL %s", url.str());
    return point;
  }

  DataPointLoader& DataHandle::getLoader() {
    // For C++ it would be enough to have 
    //   static DataPointLoader loader;
    // But Java sometimes does not destroy objects causing
    // PluginsFactory destructor loop forever waiting for
    // plugins to exit.
    static DataPointLoader* loader = NULL;
    if(!loader) {
      loader = new DataPointLoader();
    }
    return *loader;
  }

} // namespace Arc
