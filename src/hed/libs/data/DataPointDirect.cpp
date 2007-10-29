#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "DataPointDirect.h"

namespace Arc {

  DataPointDirect::DataPointDirect(const URL& url) : DataPoint(url) {
    /* connection is not initialised here - only if request is made */
    no_checks = false;
    reading = false;
    writing = false;
    is_secure = false;
    force_secure = true;
    force_passive = false;
    allow_out_of_order = false; // safe setting
    failure_code = common_failure;
    range_start = 0;
    range_end = 0;
  }

  DataPointDirect::~DataPointDirect() {
    /*  Stop any transfer - use this as an example in derived methods */
    stop_reading();
    stop_writing();
    /* !!!!!!!!!!! destroy activated handles, etc here !!!!!!!! */
    deinit_handle();
  }

  bool DataPointDirect::analyze(analyze_t& arg) {
    if(!url)
      return false;
    arg.bufnum = stringtoi(url.Option("threads"));
    if(arg.bufnum < 1)
      arg.bufnum = 1;
    if(arg.bufnum > MAX_PARALLEL_STREAMS)
      arg.bufnum = MAX_PARALLEL_STREAMS;
    arg.bufsize = stringtol(url.Option("blocksize"));
    if(arg.bufsize < 0)
      arg.bufsize = 0;
    if(arg.bufsize > MAX_BLOCK_SIZE)
      arg.bufsize = MAX_BLOCK_SIZE;
    arg.cache = (url.Option("cache", "yes") == "yes");
    arg.readonly = (url.Option("readonly", "yes") == "yes");
    arg.local = false;
    return true;
  }

  bool DataPointDirect::init_handle() {
    if(!url)
      return false;
    cacheable = (url.Option("cache", "yes") == "yes");
    linkable = (url.Option("readonly", "yes") == "yes");
    out_of_order(out_of_order());
    transfer_streams = 1;
    if(allow_out_of_order) {
      transfer_streams = stringtoi(url.Option("threads"));
      if(transfer_streams < 1)
        transfer_streams = 1;
      if(transfer_streams > MAX_PARALLEL_STREAMS)
        transfer_streams = MAX_PARALLEL_STREAMS;
    }
    return true;
  }

  bool DataPointDirect::deinit_handle() {
    return true;
  }

  bool DataPointDirect::start_reading(DataBufferPar&) {
    failure_code = common_failure;
    failure_description = "";
    if(reading || writing || !url)
      return false;
    if(!init_handle())
      return false;
    reading = true;
    return true;
  }

  bool DataPointDirect::stop_reading() {
    if(!reading)
      return false;
    reading = false;
    return true;
  }

  bool DataPointDirect::start_writing(DataBufferPar&, DataCallback*) {
    failure_code = common_failure;
    failure_description = "";
    if(reading || writing || !url)
      return false;
    if(!init_handle())
      return false;
    writing = true;
    return true;
  }

  bool DataPointDirect::stop_writing() {
    if(!writing)
      return false;
    writing = false;
    return true;
  }

  bool DataPointDirect::list_files(std::list<FileInfo>&, bool) {
    failure_code = common_failure;
    failure_description = "";
    if(reading || writing || !url)
      return false;
    if(!init_handle())
      return false;
    return true;
  }

  bool DataPointDirect::check() {
    failure_code = common_failure;
    failure_description = "";
    if(reading || writing || !url)
      return false;
    if(!init_handle())
      return false;
    return true;
  }

  bool DataPointDirect::remove() {
    failure_code = common_failure;
    if(reading || writing || !url)
      return false;
    if(!init_handle())
      return false;
    return true;
  }

  void DataPointDirect::out_of_order(bool val) {
    allow_out_of_order = val;
  }

  bool DataPointDirect::out_of_order() {
    return false;
  }

  void DataPointDirect::additional_checks(bool val) {
    no_checks = !val;
  }

  bool DataPointDirect::additional_checks() {
    return !no_checks;
  }

  void DataPointDirect::secure(bool val) {
    force_secure = val;
  }

  bool DataPointDirect::secure() {
    return is_secure;
  }

  void DataPointDirect::passive(bool val) {
    force_passive = val;
  }

  DataPointDirect::failure_reason_t DataPointDirect::failure_reason() {
    return failure_code;
  }

  std::string DataPointDirect::failure_text() {
    return failure_description;
  }

  void DataPointDirect::range(unsigned long long int start,
                              unsigned long long int end) {
    range_start = start;
    range_end = end;
  }

  const URL& DataPointDirect::current_location() const {
    return url;
  }

  const std::string& DataPointDirect::current_meta_location() const {
    static const std::string empty;
    return empty;
  }

} // namespace Arc
