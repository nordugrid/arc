#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>

#include <arc/Logger.h>

#include "DataPoint.h"

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url) : url(url),
                                         size(-1),
                                         created(-1),
                                         valid(-1),
                                         triesleft(5),
                                         bufsize(-1),
                                         bufnum(1),
                                         cache(true),
                                         local(false),
                                         readonly(true) {}

  const URL& DataPoint::base_url() const {
    return url;
  }

  std::string DataPoint::str() const {
    return url.str();
  }

  int DataPoint::GetTries() const {
    return triesleft;
  }

  void DataPoint::SetTries(const int n) {
    triesleft = std::max(0, n);
  }

} // namespace Arc
