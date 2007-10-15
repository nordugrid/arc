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
   protected:
    virtual bool init_handle();
    virtual bool deinit_handle();

   public:
    /// Constructor
    /// \param url URL.
    DataPointDirect(const URL& url);
    /// Destructor. No comments.
    virtual ~DataPointDirect();

    virtual bool meta() const {
      return false;
    };

    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading();
    virtual bool stop_writing();

    virtual bool analyze(analyze_t& arg);
    virtual bool check();

    virtual bool remove();
    virtual bool list_files(std::list <FileInfo>& files, bool resolve = true);

    virtual bool out_of_order();
    virtual void out_of_order(bool v);
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
    virtual bool meta_resolve(bool) { return false; };
    virtual bool meta_preregister(bool, bool force = false) { return false; };
    virtual bool meta_postregister(bool) { return false; };
    virtual bool meta_preunregister(bool) { return false; };
    virtual bool meta_unregister(bool all) { return false; };
    virtual bool get_info(FileInfo& fi) { return false; };
    virtual bool accepts_meta() { return false; };
    virtual bool provides_meta() { return false; };
    virtual bool meta_stored() { return false; };
    virtual bool local() const { return false; };
    virtual const URL& current_location() const;
    virtual const std::string& current_meta_location() const;
    virtual bool next_location() { return false; };
    virtual bool have_location() const { return false; };
    virtual bool have_locations() const { return false; };
    virtual bool add_location(const std::string&, const URL&) { return false; };
    virtual bool remove_location() { return false; };

   protected:
    DataBufferPar *buffer;
    bool cacheable;
    bool linkable;
    bool is_secure;
    bool force_secure;
    bool force_passive;
    bool reading;
    bool writing;
    bool no_checks;
    /* true if allowed to produce scattered data (non-sequetial offsets) */
    bool allow_out_of_order;
    int transfer_streams;
    unsigned long long int range_start;
    unsigned long long int range_end;

    failure_reason_t failure_code;
    std::string failure_description;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTDIRECT_H__
