// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/data/DataPointIndex.h>

namespace Arc {

  DataPointIndex::DataPointIndex(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg)
    : DataPoint(url, usercfg, transfer_url, parg),
      resolved(false),
      registered(false),
      h(NULL) {
    location = locations.end();
  }

  DataPointIndex::~DataPointIndex() {
    if (h)
      delete h;
  }

  const URL& DataPointIndex::CurrentLocation() const {
    static const URL empty;
    if (locations.end() == location)
      return empty;
    return *location;
  }

  const std::string& DataPointIndex::CurrentLocationMetadata() const {
    static const std::string empty;
    if (locations.end() == location)
      return empty;
    return location->Name();
  }

  DataPoint* DataPointIndex::CurrentLocationHandle() const {
    if (!h) return NULL;
    return &(**h);
  }

  DataStatus DataPointIndex::CompareLocationMetadata() const {
    if (h && *h) {
      FileInfo fileinfo;
      DataPoint::DataPointInfoType verb = (DataPoint::DataPointInfoType)
                                          (DataPoint::INFO_TYPE_TIMES |
                                           DataPoint::INFO_TYPE_CONTENT);
      DataStatus res = (*h)->Stat(fileinfo, verb);
      if (!res.Passed())
        return res;
      if (!CompareMeta(*(*h)))
        return DataStatus::InconsistentMetadataError;
    }
    return DataStatus::Success;
  }  
    
  bool DataPointIndex::HaveLocations() const {
    return (locations.size() != 0);
  }

  bool DataPointIndex::LocationValid() const {
    if (triesleft <= 0)
      return false;
    if (locations.end() == location)
      return false;
    return true;
  }

  bool DataPointIndex::LastLocation() {
    if (location == locations.end())
      return true;
    bool last = false;
    if (++location == locations.end())
      last = true;
    location--;
    return last;
  }

  void DataPointIndex::SetHandle(void) {
    // TODO: pass various options from old handler to new
    if (h)
      delete h;
    if (locations.end() != location) {
      h = new DataHandle(*location, usercfg);
      if (!h || !(*h)) {
        logger.msg(WARNING, "Can't handle location %s", location->str());
        delete h;
        h = NULL;
        RemoveLocation();
      }
      else
        (*h)->SetMeta(*this);
    }
    else
      h = NULL;
  }

  bool DataPointIndex::NextLocation() {
    if (!LocationValid()) {
      --triesleft;
      return false;
    }
    ++location;
    if (locations.end() == location)
      if (--triesleft > 0)
        location = locations.begin();
    SetHandle();
    return LocationValid();
  }

  DataStatus DataPointIndex::RemoveLocation() {
    if (locations.end() == location)
      return DataStatus::NoLocationError;
    location = locations.erase(location);
    if (locations.end() == location)
      location = locations.begin();
    SetHandle();
    return DataStatus::Success;
  }

  DataStatus DataPointIndex::RemoveLocations(const DataPoint& p_) {
    if (!p_.IsIndex())
      return DataStatus::Success;
    const DataPointIndex& p = dynamic_cast<const DataPointIndex&>(p_);
    std::list<URLLocation>::iterator p_int;
    std::list<URLLocation>::const_iterator p_ext;
    for (p_ext = p.locations.begin(); p_ext != p.locations.end(); ++p_ext)
      for (p_int = locations.begin(); p_int != locations.end();)
        // Compare URLs
        if (*p_int == *p_ext)
          if (location == p_int) {
            p_int = locations.erase(p_int);
            location = p_int;
          }
          else
            p_int = locations.erase(p_int);
        else
          ++p_int;
    if (locations.end() == location)
      location = locations.begin();
    SetHandle();
    return DataStatus::Success;
  }

  DataStatus DataPointIndex::ClearLocations() {
    locations.clear();
    location = locations.end();
    SetHandle();
    return DataStatus::Success;
  }



  DataStatus DataPointIndex::AddLocation(const URL& url,
                                         const std::string& meta) {
    logger.msg(DEBUG, "Add location: url: %s", url.str());
    logger.msg(DEBUG, "Add location: metadata: %s", meta);
    for (std::list<URLLocation>::iterator i = locations.begin();
         i != locations.end(); ++i)
      if ((i->Name() == meta) && (url == (*i)))
        return DataStatus::LocationAlreadyExistsError;
    locations.push_back(URLLocation(url, meta));
    if(locations.end() == location) {
      location = locations.begin();
      SetHandle();
    }
    return DataStatus::Success;
  }

  void DataPointIndex::SortLocations(const std::string& pattern, const URLMap& url_map) {

    if (locations.size() < 2 || (pattern.empty() && !url_map))
      return;
    std::list<URLLocation> sorted_locations;

    // sort according to URL map first
    if (url_map) {
      logger.msg(VERBOSE, "Sorting replicas according to URL map");
      for (std::list<URLLocation>::iterator l = locations.begin(); l != locations.end(); ++l) {
        URL mapped_url = *l;
        if (url_map.map(mapped_url)) {
          logger.msg(VERBOSE, "Replica %s is mapped", l->str());
          sorted_locations.push_back(*l);
        }
      }
    }

    // sort according to preferred pattern
    if (!pattern.empty()) {
      logger.msg(VERBOSE, "Sorting replicas according to preferred pattern %s", pattern);

      // go through each pattern in pattern - if match then add to sorted list
      std::list<std::string> patterns;
      Arc::tokenize(pattern, patterns, "|");

      for (std::list<std::string>::iterator p = patterns.begin(); p != patterns.end(); ++p) {
        std::string to_match = *p;
        bool match_host = false;
        if (to_match.rfind('$') == to_match.length()-1) { // only match host
          to_match.erase(to_match.length()-1);
          match_host = true;
        }
        bool exclude = false;
        if (to_match.find('!') == 0) {
          to_match.erase(0, 1);
          exclude = true;
        }
        for (std::list<URLLocation>::iterator l = locations.begin(); l != locations.end();) {
          if (match_host) {
            if ((l->Host().length() >= to_match.length()) &&
                (l->Host().rfind(to_match) == (l->Host().length() - to_match.length()))) {
              if (exclude) {
                logger.msg(VERBOSE, "Excluding replica %s matching pattern !%s", l->str(), to_match);
                l = locations.erase(l);
                continue;
              }
              // check if already present
              bool present = false;
              for (std::list<URLLocation>::iterator j = sorted_locations.begin();j!=sorted_locations.end();++j) {
                if (*j == *l)
                  present = true;
              }
              if (!present) {
                logger.msg(VERBOSE, "Replica %s matches host pattern %s", l->str(), to_match);
                sorted_locations.push_back(*l);
              }
            }
          }
          else if (l->str().find(*p) != std::string::npos) {
            if (exclude) {
              logger.msg(VERBOSE, "Excluding replica %s matching pattern !%s", l->str(), to_match);
              l = locations.erase(l);
              continue;
            }
            // check if already present
            bool present = false;
            for (std::list<URLLocation>::iterator j = sorted_locations.begin();j!=sorted_locations.end();++j) {
              if (*j == *l)
                present = true;
            }
            if (!present) {
              logger.msg(VERBOSE, "Replica %s matches pattern %s", l->str(), *p);
              sorted_locations.push_back(*l);
            }
          }
          ++l;
        }
      }
    }
    // add anything left
    for (std::list<URLLocation>::iterator i = locations.begin();i!=locations.end();++i) {
      bool present = false;
      for (std::list<URLLocation>::iterator j = sorted_locations.begin();j!=sorted_locations.end();++j) {
        if (*j == *i)
          present = true;
      }
      if (!present) {
        logger.msg(VERBOSE, "Replica %s doesn't match preferred pattern or URL map", i->str());
        sorted_locations.push_back(*i);
      }
    }
    locations = sorted_locations;
    location = locations.begin();
    SetHandle();
  }


  void DataPointIndex::SetTries(const int n) {
    triesleft = std::max(0, n);
    if (triesleft == 0)
      location = locations.end();
    else if (locations.end() == location)
      location = locations.begin();
    SetHandle();
  }

  bool DataPointIndex::IsIndex() const {
    return true;
  }

  bool DataPointIndex::IsStageable() const {
    if (!h || !*h)
      return false;
    return (*h)->IsStageable();
  }

  bool DataPointIndex::AcceptsMeta() const {
    return true;
  }

  bool DataPointIndex::ProvidesMeta() const {
    return true;
  }

  void DataPointIndex::SetMeta(const DataPoint& p) {
    if (!CheckSize())
      SetSize(p.GetSize());
    if (!CheckCheckSum())
      SetCheckSum(p.GetCheckSum());
    if (!CheckModified())
      SetModified(p.GetModified());
    if (!CheckValid())
      SetValid(p.GetValid());
    // set for current handle
    if (h && *h)
      (*h)->SetMeta(p);
  }

  void DataPointIndex::SetCheckSum(const std::string& val) {
    checksum = val;
    if (h && *h)
      (*h)->SetCheckSum(val);
  }

  void DataPointIndex::SetSize(const unsigned long long int val) {
    size = val;
    if (h && *h)
      (*h)->SetSize(val);
  }

  bool DataPointIndex::Registered() const {
    return registered;
  }

  DataStatus DataPointIndex::StartReading(DataBuffer& buffer) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->StartReading(buffer);
  }

  DataStatus DataPointIndex::PrepareReading(unsigned int timeout,
                                            unsigned int& wait_time) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->PrepareReading(timeout, wait_time);
  }

  DataStatus DataPointIndex::PrepareWriting(unsigned int timeout,
                                            unsigned int& wait_time) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->PrepareWriting(timeout, wait_time);
  }

  DataStatus DataPointIndex::StartWriting(DataBuffer& buffer,
                                          DataCallback *cb) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->StartWriting(buffer, cb);
  }

  DataStatus DataPointIndex::StopReading() {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->StopReading();
  }

  DataStatus DataPointIndex::StopWriting() {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->StopWriting();
  }

  DataStatus DataPointIndex::FinishReading(bool error) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->FinishReading(error);
  }

  DataStatus DataPointIndex::FinishWriting(bool error) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->FinishWriting(error);
  }

  std::vector<URL> DataPointIndex::TransferLocations() const {
    if (!h || !*h) {
      std::vector<URL> empty_vector;
      return empty_vector;
    }
    return (*h)->TransferLocations();
  }

  void DataPointIndex::ClearTransferLocations() {
    if (h && *h) (*h)->ClearTransferLocations();
  }

  DataStatus DataPointIndex::Check(bool check_meta) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->Check(check_meta);
  }

  long long int DataPointIndex::BufSize() const {
    if (!h || !*h)
      return -1;
    return (*h)->BufSize();
  }

  int DataPointIndex::BufNum() const {
    if (!h || !*h)
      return 1;
    return (*h)->BufNum();
  }

  bool DataPointIndex::Local() const {
    if (!h || !*h)
      return false;
    return (*h)->Local();
  }

  bool DataPointIndex::ReadOnly() const {
    if (!h || !*h)
      return true;
    return (*h)->ReadOnly();
  }

  DataStatus DataPointIndex::Remove() {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->Remove();
  }

  void DataPointIndex::ReadOutOfOrder(bool v) {
    if (h && *h)
      (*h)->ReadOutOfOrder(v);
  }

  bool DataPointIndex::WriteOutOfOrder() {
    if (!h || !*h)
      return false;
    return (*h)->WriteOutOfOrder();
  }

  void DataPointIndex::SetAdditionalChecks(bool v) {
    if (h && *h)
      (*h)->SetAdditionalChecks(v);
  }

  bool DataPointIndex::GetAdditionalChecks() const {
    if (!h || !*h)
      return false;
    return (*h)->GetAdditionalChecks();
  }

  void DataPointIndex::SetSecure(bool v) {
    if (h && *h)
      (*h)->SetSecure(v);
  }

  bool DataPointIndex::GetSecure() const {
    if (!h || !*h)
      return false;
    return (*h)->GetSecure();
  }

  DataPoint::DataPointAccessLatency DataPointIndex::GetAccessLatency() const {
    if (!h || !*h)
      return ACCESS_LATENCY_ZERO;
    return (*h)->GetAccessLatency();

  }

  void DataPointIndex::Passive(bool v) {
    if (h && *h)
      (*h)->Passive(v);
  }

  void DataPointIndex::Range(unsigned long long int start,
                             unsigned long long int end) {
    if (h && *h)
      (*h)->Range(start, end);
  }

  int DataPointIndex::AddCheckSumObject(CheckSum* /* cksum */) {
    return -1;
  }

  const CheckSum* DataPointIndex::GetCheckSumObject(int /* index */) const {
    return NULL;
  }

} // namespace Arc
