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
    bool is_metaexisting;
    bool is_resolved;
   public:
    DataPointIndex(const URL& url);
    virtual ~DataPointIndex() {};
    virtual bool get_info(FileInfo& fi);

    virtual const URL& current_location() const;
    virtual const std::string& current_meta_location() const;

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

    // Not supported for index data points:
    virtual bool start_reading(DataBufferPar&) { return false; };
    virtual bool start_writing(DataBufferPar&,
                               DataCallback* = NULL) { return false; };
    virtual bool stop_reading() { return false; };
    virtual bool stop_writing() { return false; };
    virtual bool analyze(analyze_t&) { return false; };
    virtual bool check() { return false; };
    virtual bool local() const { return false; };
    virtual bool remove() { return false; };
    virtual bool out_of_order() { return false; };
    virtual void out_of_order(bool) {};
    virtual void additional_checks(bool) {};
    virtual bool additional_checks() { return false; };
    virtual void secure(bool) {};
    virtual bool secure() { return false; };
    virtual void passive(bool) {};
    virtual failure_reason_t failure_reason() { return common_failure; };
    std::string failure_text() { return ""; };
    virtual void range(unsigned long long int = 0,
                       unsigned long long int = 0) {};
  };

} // namespace Arc

#endif // __ARC_DATAPOINTINDEX_H__
