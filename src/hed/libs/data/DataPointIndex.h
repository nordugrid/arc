#ifndef __ARC_DATAPOINTINDEX_H__
#define __ARC_DATAPOINTINDEX_H__

#include <string>
#include <list>

#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>

namespace Arc {

  /// Complements DataPoint with attributes common for meta-URLs
  /** It should never be used directly. Instead inherit from it to provide
    class for specific Indexing Service. */
  class DataPointIndex : public DataPoint {
   protected:
    /// List of locations at which file can be probably found.
    std::list<URLLocation> locations;
    std::list<URLLocation>::iterator location;
    DataHandle h;
    bool is_metaexisting;
    bool is_resolved;
   public:
    DataPointIndex(const URL& url);
    virtual ~DataPointIndex() {};

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

    // the following are relayed to the current location
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading();
    virtual bool stop_writing();

    virtual bool check();
    virtual bool Local() const;

    virtual bool remove();

    virtual void out_of_order(bool v);
    virtual bool out_of_order();
    virtual void additional_checks(bool v);
    virtual bool additional_checks();

    virtual void secure(bool v);
    virtual bool secure();

    virtual void passive(bool v);

    virtual failure_reason_t failure_reason();
    virtual std::string failure_text();

    virtual void range(unsigned long long int start = 0,
                       unsigned long long int end = 0);
  };

} // namespace Arc

#endif // __ARC_DATAPOINTINDEX_H__
