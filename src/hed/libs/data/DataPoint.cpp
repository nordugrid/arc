#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>

#include <arc/Logger.h>

#include "DataPoint.h"

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url) : url(url),
                                         meta_size_(-1),
                                         meta_created_(-1),
                                         meta_validtill_(-1),
                                         tries_left(5) {}

  std::string DataPoint::empty_string_;
  URL DataPoint::empty_url_;

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

} // namespace Arc
