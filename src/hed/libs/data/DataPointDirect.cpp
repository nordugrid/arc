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
    bufnum = stringtoi(url.Option("threads"));
    if(bufnum < 1)
      bufnum = 1;
    if(bufnum > MAX_PARALLEL_STREAMS)
      bufnum = MAX_PARALLEL_STREAMS;
    bufsize = stringtol(url.Option("blocksize"));
    if(bufsize < 0)
      bufsize = 0;
    if(bufsize > MAX_BLOCK_SIZE)
      bufsize = MAX_BLOCK_SIZE;
    cache = (url.Option("cache", "yes") == "yes");
    readonly = (url.Option("readonly", "yes") == "yes");
    local = false;
  }

  DataPointDirect::~DataPointDirect() {}

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
