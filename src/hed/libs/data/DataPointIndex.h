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
    /// List of locations at which file can be probably found.
    std::list<URLLocation> locations;
    std::list<URLLocation>::iterator location;
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
        return *location;
      return empty_url_;
    };

    virtual const std::string& current_meta_location() const {
      if(location != locations.end())
        return location->Name();
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
