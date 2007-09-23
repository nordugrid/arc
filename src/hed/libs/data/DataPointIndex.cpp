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

  void DataPointIndex::fix_unregistered(bool all) {
    if(all) {
      is_metaexisting = false;
      locations.clear();
      location = locations.end();
    }
    else {
      location = locations.erase(location);
      if(location == locations.end()) location = locations.begin();
    }
  }

  bool DataPointIndex::get_info(FileInfo& fi) {
    if(!meta_resolve(true)) {
      return false;
    }
    fi = url.Path();
    for(std::list<Location>::iterator i = locations.begin();
        i != locations.end(); ++i)
      fi.AddURL(i->url);
    if(meta_size_available())
      fi.SetSize(meta_size_);
    if(meta_checksum_available())
      fi.SetCheckSum(meta_checksum_);
    if(meta_created_available())
      fi.SetCreated(meta_created_);
    if(meta_validtill_available())
      fi.SetValid(meta_validtill_);
    fi.SetType(FileInfo::file_type_file);
    return true;
  }

  bool DataPointIndex::have_locations() const {
    if(!url) return false;
    return (locations.size() != 0);
  }

  bool DataPointIndex::have_location() const {
    if(!url) return false;
    if(tries_left <= 0) return false;
    std::list<Location>::const_iterator l = location;
    if(l == locations.end()) return false;
    return true;
  }

  bool DataPointIndex::next_location() {
    if(tries_left <= 0) return false;
    if(location == locations.end()) return false;
    ++location;
    if(location == locations.end()) {
      tries_left--;
      if(tries_left <= 0) return false;
      location = locations.begin();
    }
    return true;
  }

  bool DataPointIndex::remove_location() {
    if(location == locations.end()) return false;
    location = locations.erase(location);
    return true;
  }

  bool DataPointIndex::remove_locations(const DataPoint& p_) {
    if(!(p_.have_locations())) return true;
    const DataPointIndex& p = dynamic_cast<const DataPointIndex &>(p_);
    std::list<Location>::iterator p_int;
    std::list<Location>::const_iterator p_ext;
    for(p_ext = p.locations.begin(); p_ext != p.locations.end(); ++p_ext) {
      for(p_int = locations.begin(); p_int != locations.end();) {
        // Compare protocol+host+port part
        if((p_int->url.ConnectionURL() == p_ext->url.ConnectionURL())) {
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
    return true;
  }

  bool DataPointIndex::add_location(const std::string& meta_loc,
                                    const URL& loc) {
    logger.msg(DEBUG, "Add location: metaname: %s", meta_loc.c_str());
    logger.msg(DEBUG, "Add location: location: %s", loc.str().c_str());
    for(std::list<Location>::iterator i = locations.begin();
        i != locations.end(); ++i) {
      if(i->meta == meta_loc) return true; // Already exists
    }
    locations.insert(locations.end(), Location(meta_loc, loc, false));
    return true;
  }

  void DataPointIndex::tries(int n) {
    if(n < 0)
      n = 0;
    tries_left = n;
    if(n == 0)
      location = locations.end();
    else if(location == locations.end())
      location = locations.begin();
  }

} // namespace Arc
