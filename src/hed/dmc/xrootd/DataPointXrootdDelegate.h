// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTXROOTDDELEGATE_H__
#define __ARC_DATAPOINTXROOTDDELEGATE_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/Utils.h>
#include <arc/data/DataPointDelegate.h>

namespace ArcDMCXrootd {

  using namespace Arc;

  class DataPointXrootdDelegate
    : public DataPointDelegate {
  public:
    DataPointXrootdDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointXrootdDelegate();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool RequiresCredentials() const;
    virtual bool RequiresCredentialsInFile() const;
    virtual bool SupportsTransfer() const;
    virtual DataStatus List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);

  };

} // namespace ArcDMCGridFTP

#endif // __ARC_DATAPOINTXROOTDDELEGATE_H__

