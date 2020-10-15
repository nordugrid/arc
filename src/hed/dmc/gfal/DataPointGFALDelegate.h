// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGFALDELEGATE_H__
#define __ARC_DATAPOINTGFALDELEGATE_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/Utils.h>
#include <arc/data/DataPointDelegate.h>

namespace ArcDMCGFAL {

  using namespace Arc;

  class DataPointGFALDelegate
    : public DataPointDelegate {
  public:
    DataPointGFALDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointGFALDelegate();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool RequiresCredentials() const;
    virtual bool WriteOutOfOrder() const;
  }
  };

} // namespace ArcDMCGFAL

#endif // __ARC_DATAPOINTGFALDELEGATE_H__

