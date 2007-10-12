#ifndef __ARC_DATAPOINTINDEX_H__
#define __ARC_DATAPOINTINDEX_H__

#include <string>
#include <list>

#include <arc/data/DataPoint.h>

namespace Arc {

  /// Complements DataPoint with attributes common for meta-URLs
  /** It should never be used directly. Instead inherit from it to provide
    class for specific Indexing Service. */
  class DataPointIndex : public DataPoint {
   protected:
    /// DataPointIndex::Location represents physical service at which
    /// files are located aka "base URL" inculding it's name (as given
    /// in Indexing Service).
    /// Currently it is used only internally by classes derived from
    /// DataPointIndex class and for printing debug information.
    class Location {
     public:
      std::string meta; // Given name of location
      URL url;          // location aka pfn aka access point
      bool existing;
      void *arg; // to be used by different pieces of soft differently
      Location() : existing(true), arg(NULL) {};
      Location(const URL& url) : url(url), existing(true), arg(NULL) {};
      Location(const std::string& meta, const URL& url,
               bool existing = true) : meta(meta), url(url),
                                       existing(existing), arg(NULL) {};
    };

    /// List of locations at which file can be probably found.
    std::list<Location> locations;
    std::list<Location>::iterator location;
   protected:
    bool is_metaexisting;
    bool is_resolved;
    void fix_unregistered(bool all);
   public:
    DataPointIndex(const URL& url);
    virtual ~DataPointIndex() {};
    virtual bool get_info(FileInfo& fi);

    virtual const URL& current_location() const {
      if(location != locations.end())
        return location->url;
      return empty_url_;
    };

    virtual const std::string& current_meta_location() const {
      if(location != locations.end())
        return location->meta;
      return empty_string_;
    };

    virtual bool next_location();
    virtual bool have_location() const;
    virtual bool have_locations() const;
    virtual bool remove_location();
    virtual bool remove_locations(const DataPoint& p);
    virtual bool add_location(const std::string& meta, const URL& loc);

    virtual bool meta() const {
      return true;
    };

    virtual bool accepts_meta() {
      return true;
    };

    virtual bool provides_meta() {
      return true;
    };

    virtual bool meta_stored() {
      return is_metaexisting;
    };

    virtual void SetTries(const int n);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTINDEX_H__
