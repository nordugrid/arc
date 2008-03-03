#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>

#include <arc/Logger.h>

#include "DataPointIndex.h"

namespace Arc {

  DataPointIndex::DataPointIndex(const URL& url) : DataPoint(url),
                                                   is_metaexisting(false),
                                                   is_resolved(false) {
    location = locations.end();
  }

  const URL& DataPointIndex::current_location() const {
    static const URL empty;
    if(location == locations.end())
      return empty;
    return *location;
  };

  const std::string& DataPointIndex::current_meta_location() const {
    static const std::string empty;
    if(location == locations.end())
      return empty;
    return location->Name();
  };

  bool DataPointIndex::have_locations() const {
    return (locations.size() != 0);
  }

  bool DataPointIndex::have_location() const {
    if(triesleft <= 0) return false;
    if(location == locations.end()) return false;
    return true;
  }

  bool DataPointIndex::next_location() {
    if(!have_location()) return false;
    ++location;
    if(location == locations.end()) {
      if(--triesleft > 0)
        location = locations.begin();
    }
    if(location != locations.end())
      h = *location;
    else
      h.Clear();
    return have_location();
  }

  bool DataPointIndex::remove_location() {
    if(location == locations.end()) return false;
    location = locations.erase(location);
    if(location != locations.end())
      h = *location;
    else
      h.Clear();
    return true;
  }

  bool DataPointIndex::remove_locations(const DataPoint& p_) {
    if(!(p_.have_locations())) return true;
    const DataPointIndex& p = dynamic_cast<const DataPointIndex &>(p_);
    std::list<URLLocation>::iterator p_int;
    std::list<URLLocation>::const_iterator p_ext;
    for(p_ext = p.locations.begin(); p_ext != p.locations.end(); ++p_ext) {
      for(p_int = locations.begin(); p_int != locations.end();) {
        // Compare protocol+host+port part
        if((p_int->ConnectionURL() == p_ext->ConnectionURL())) {
          if(location == p_int) {
            p_int = locations.erase(p_int);
            location = p_int;
          }
          else {
            p_int = locations.erase(p_int);
          }
          continue;
        }
        ++p_int;
      }
    }
    if(location == locations.end())
      location = locations.begin();
    if(location != locations.end())
      h = *location;
    else
      h.Clear();
    return true;
  }

  bool DataPointIndex::add_location(const std::string& meta_loc,
                                    const URL& loc) {
    logger.msg(DEBUG, "Add location: metaname: %s", meta_loc.c_str());
    logger.msg(DEBUG, "Add location: location: %s", loc.str().c_str());
    for(std::list<URLLocation>::iterator i = locations.begin();
        i != locations.end(); ++i) {
      if(i->Name() == meta_loc) return true; // Already exists
    }
    locations.insert(locations.end(), URLLocation(loc, meta_loc));
    return true;
  }

  void DataPointIndex::SetTries(const int n) {
    triesleft = std::max(0, n);
    if(triesleft == 0)
      location = locations.end();
    else if(location == locations.end())
      location = locations.begin();
    if(location != locations.end())
      h = *location;
    else
      h.Clear();
  }

  bool DataPointIndex::start_reading(DataBufferPar& buffer) {
    if(!h) return false;
    return h->start_reading(buffer);
  }

  bool DataPointIndex::start_writing(DataBufferPar& buffer, DataCallback *cb) {
    if(!h) return false;
    return h->start_writing(buffer, cb);
  }

  bool DataPointIndex::stop_reading() {
    if(!h) return false;
    return h->stop_reading();
  }

  bool DataPointIndex::stop_writing() {
    if(!h) return false;
    return h->stop_writing();
  }

  bool DataPointIndex::check() {
    if(!h) return false;
    return h->check();
  }

  bool DataPointIndex::Local() const {
    if(!h) return false;
    return h->Local();
  }

  bool DataPointIndex::remove() {
    if(!h) return false;
    return h->remove();
  }

  void DataPointIndex::out_of_order(bool v) {
    if(h) h->out_of_order(v);
  }

  bool DataPointIndex::out_of_order() {
    if(!h) return false;
    return h->out_of_order();
  }

  void DataPointIndex::additional_checks(bool v) {
    if(h) h->additional_checks(v);
  }

  bool DataPointIndex::additional_checks() {
    if(!h) return false;
    return h->additional_checks();
  }

  void DataPointIndex::secure(bool v) {
    if(h) h->secure(v);
  }

  bool DataPointIndex::secure() {
    if(!h) return false;
    return h->secure();
  }

  void DataPointIndex::passive(bool v) {
    if(h) h->passive(v);
  }

  DataPointIndex::failure_reason_t DataPointIndex::failure_reason() {
    if(!h) return common_failure;
    return h->failure_reason();
  }

  std::string DataPointIndex::failure_text() {
    if(!h) return "";
    return h->failure_text();
  }

  void DataPointIndex::range(unsigned long long int start,
                             unsigned long long int end) {
    if(h) h->range(start, end);
  }

} // namespace Arc
