#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>

#include <arc/Logger.h>

#include "DataPointIndex.h"

namespace Arc {

  DataPointIndex::DataPointIndex(const URL& url) : DataPoint(url),
                                                   registered(false) {
    location = locations.end();
  }

  DataPointIndex::~DataPointIndex() {}

  const URL& DataPointIndex::CurrentLocation() const {
    static const URL empty;
    if (location == locations.end())
      return empty;
    return *location;
  }

  const std::string& DataPointIndex::CurrentLocationMetadata() const {
    static const std::string empty;
    if (location == locations.end())
      return empty;
    return location->Name();
  }

  bool DataPointIndex::HaveLocations() const {
    return (locations.size() != 0);
  }

  bool DataPointIndex::LocationValid() const {
    if (triesleft <= 0)
      return false;
    if (location == locations.end())
      return false;
    return true;
  }

  bool DataPointIndex::NextLocation() {
    if (!LocationValid())
      return false;
    ++location;
    if (location == locations.end()) {
      if (--triesleft > 0)
        location = locations.begin();
    }
    if (location != locations.end())
      h = *location;
    else
      h.Clear();
    return LocationValid();
  }

  DataStatus DataPointIndex::RemoveLocation() {
    if (location == locations.end())
      return DataStatus::NoLocationError;
    location = locations.erase(location);
    if (location == locations.end())
      location = locations.begin();
    if (location != locations.end())
      h = *location;
    else
      h.Clear();
    return DataStatus::Success;
  }

  DataStatus DataPointIndex::RemoveLocations(const DataPoint& p_) {
    if (!p_.IsIndex())
      return DataStatus::Success;
    const DataPointIndex& p = dynamic_cast<const DataPointIndex &>(p_);
    std::list<URLLocation>::iterator p_int;
    std::list<URLLocation>::const_iterator p_ext;
    for (p_ext = p.locations.begin(); p_ext != p.locations.end(); ++p_ext) {
      for (p_int = locations.begin(); p_int != locations.end();) {
        // Compare protocol+host+port part
        if ((p_int->ConnectionURL() == p_ext->ConnectionURL())) {
          if (location == p_int) {
            p_int = locations.erase(p_int);
            location = p_int;
          }
          else
            p_int = locations.erase(p_int);
        }
        else
          ++p_int;
      }
    }
    if (location == locations.end())
      location = locations.begin();
    if (location != locations.end())
      h = *location;
    else
      h.Clear();
    return DataStatus::Success;
  }

  DataStatus DataPointIndex::AddLocation(const URL& url,
                                   const std::string& meta) {
    logger.msg(DEBUG, "Add location: url: %s", url.str().c_str());
    logger.msg(DEBUG, "Add location: metadata: %s", meta.c_str());
    for (std::list<URLLocation>::iterator i = locations.begin();
        i != locations.end(); ++i)
      if (i->Name() == meta)
        return DataStatus::LocationAlreadyExistsError;
    locations.push_back(URLLocation(url, meta));
    return DataStatus::Success;
  }

  void DataPointIndex::SetTries(const int n) {
    triesleft = std::max(0, n);
    if (triesleft == 0)
      location = locations.end();
    else if (location == locations.end())
      location = locations.begin();
    if (location != locations.end())
      h = *location;
    else
      h.Clear();
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

  DataStatus DataPointIndex::StartReading(DataBufferPar& buffer) {
    if (!h)
      return DataStatus::NoLocationError;
    return h->StartReading(buffer);
  }

  DataStatus DataPointIndex::StartWriting(DataBufferPar& buffer,
                                          DataCallback *cb) {
    if (!h)
      return DataStatus::NoLocationError;
    return h->StartWriting(buffer, cb);
  }

  DataStatus DataPointIndex::StopReading() {
    if (!h)
      return DataStatus::NoLocationError;
    return h->StopReading();
  }

  DataStatus DataPointIndex::StopWriting() {
    if (!h)
      return DataStatus::NoLocationError;
    return h->StopWriting();
  }

  DataStatus DataPointIndex::Check() {
    if (!h)
      return DataStatus::NoLocationError;
    return h->Check();
  }

  unsigned long long int DataPointIndex::BufSize() const {
    if (!h)
      return -1;
    return h->BufSize();
  }

  int DataPointIndex::BufNum() const {
    if (!h)
      return 1;
    return h->BufNum();
  }

  bool DataPointIndex::Cache() const {
    if (!h)
      return false;
    return h->Cache();
  }

  bool DataPointIndex::Local() const {
    if (!h)
      return false;
    return h->Local();
  }

  bool DataPointIndex::ReadOnly() const {
    if (!h)
      return true;
    return h->ReadOnly();
  }

  DataStatus DataPointIndex::Remove() {
    if (!h)
      return DataStatus::NoLocationError;
    return h->Remove();
  }

  void DataPointIndex::ReadOutOfOrder(bool v) {
    if (h)
      h->ReadOutOfOrder(v);
  }

  bool DataPointIndex::WriteOutOfOrder() {
    if (!h)
      return false;
    return h->WriteOutOfOrder();
  }

  void DataPointIndex::SetAdditionalChecks(bool v) {
    if (h)
      h->SetAdditionalChecks(v);
  }

  bool DataPointIndex::GetAdditionalChecks() const {
    if (!h)
      return false;
    return h->GetAdditionalChecks();
  }

  void DataPointIndex::SetSecure(bool v) {
    if (h)
      h->SetSecure(v);
  }

  bool DataPointIndex::GetSecure() const {
    if (!h)
      return false;
    return h->GetSecure();
  }

  void DataPointIndex::Passive(bool v) {
    if (h)
      h->Passive(v);
  }

  void DataPointIndex::Range(unsigned long long int start,
                             unsigned long long int end) {
    if (h)
      h->Range(start, end);
  }

} // namespace Arc
