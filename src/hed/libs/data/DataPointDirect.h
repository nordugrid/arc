#ifndef __ARC_DATAPOINTDIRECT_H__
#define __ARC_DATAPOINTDIRECT_H__

#include <string>
#include <list>

#include "DataPoint.h"

#define MAX_PARALLEL_STREAMS 20
#define MAX_BLOCK_SIZE (1024 * 1024)

namespace Arc {

  class DataBufferPar;
  class DataCallback;

  /// This is kind of generalized file handle. 
  /** Differently from file handle it does not support operations 
    read() and write(). Instead it initiates operation and uses object
    of class DataBufferPar to pass actual data. It also provides
    other operations like querying parameters of remote object.
    It is used by higher-level classes DataMove and DataMovePar
    to provide data transfer service for application. */
  class DataPointDirect : public DataPoint {
   public:
    /// Constructor
    /// \param url URL.
    DataPointDirect(const URL& url);
    /// Destructor. No comments.
    virtual ~DataPointDirect();

    virtual bool meta() const {
      return false;
    };

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

    // Not supported for direct data points:
    virtual bool meta_resolve(bool) { return true; };
    virtual bool meta_preregister(bool, bool force = false) { return true; };
    virtual bool meta_postregister(bool) { return true; };
    virtual bool meta_preunregister(bool) { return true; };
    virtual bool meta_unregister(bool all) { return true; };
    virtual bool accepts_meta() { return false; };
    virtual bool provides_meta() { return false; };
    virtual bool meta_stored() { return false; };
    virtual const URL& current_location() const;
    virtual const std::string& current_meta_location() const;
    virtual bool next_location() { if(triesleft > 0) --triesleft; return (triesleft > 0); };
    virtual bool have_location() const { return (triesleft > 0); };
    virtual bool have_locations() const { return true; };
    virtual bool add_location(const std::string&, const URL&) { return false; };
    virtual bool remove_location() { return false; };
    virtual bool remove_locations(const DataPoint& p) { return false; };

   protected:
    DataBufferPar *buffer;
    bool linkable;
    bool is_secure;
    bool force_secure;
    bool force_passive;
    bool reading;
    bool writing;
    bool no_checks;
    /* true if allowed to produce scattered data (non-sequetial offsets) */
    bool allow_out_of_order;
    unsigned long long int range_start;
    unsigned long long int range_end;

    failure_reason_t failure_code;
    std::string failure_description;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTDIRECT_H__
