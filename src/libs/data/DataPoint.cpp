#include <list>

#include "common/Logger.h"

#include "DataPoint.h"

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url) : url(url),
					 meta_size_valid(false),
					 meta_checksum_valid(false),
					 meta_created_valid(false),
					 meta_validtill_valid(false),
					 tries_left(5) {}

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
    fi.name = url.Path();
    for(std::list<Location>::iterator i = locations.begin();
        i != locations.end(); ++i) {
      fi.urls.push_back(i->url);
    }
    if(meta_size_valid) {
      fi.size = meta_size_;
      fi.size_available = true;
    }
    if(meta_checksum_valid) {
      fi.checksum = meta_checksum_;
      fi.checksum_available = true;
    }
    if(meta_created_valid) {
      fi.created = meta_created_;
      fi.created_available = true;
    }
    if(meta_validtill_valid) {
      fi.valid = meta_validtill_;
      fi.valid_available = true;
    }
    fi.type = FileInfo::file_type_file;
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

  const URL& DataPoint::base_url() const {
    return url;
  }

  int DataPoint::tries() {
    return tries_left;
  }

  void DataPoint::tries(int n) {
    if(n < 0)
      n = 0;
    tries_left = n;
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
