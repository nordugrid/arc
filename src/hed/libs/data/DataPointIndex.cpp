// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/Logger.h>
#include <arc/data/DataPointIndex.h>

namespace Arc {

  DataPointIndex::DataPointIndex(const URL& url, const UserConfig& usercfg)
    : DataPoint(url, usercfg),
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

  void DataPointIndex::SetHandle(void) {
    // TODO: pass various options from old handler to new
    if (h)
      delete h;
    if (locations.end() != location)
      h = new DataHandle(*location, usercfg);
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

  bool DataPointIndex::AcceptsMeta() {
    return true;
  }

  bool DataPointIndex::ProvidesMeta() {
    return true;
  }

  bool DataPointIndex::Registered() const {
    return registered;
  }

  DataStatus DataPointIndex::StartReading(DataBuffer& buffer) {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->StartReading(buffer);
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

  DataStatus DataPointIndex::Check() {
    if (!h || !*h)
      return DataStatus::NoLocationError;
    return (*h)->Check();
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

  void DataPointIndex::Passive(bool v) {
    if (h && *h)
      (*h)->Passive(v);
  }

  void DataPointIndex::Range(unsigned long long int start,
                             unsigned long long int end) {
    if (h && *h)
      (*h)->Range(start, end);
  }

} // namespace Arc
