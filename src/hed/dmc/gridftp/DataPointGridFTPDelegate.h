// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTGRIDFTPDELEGATE_H__
#define __ARC_DATAPOINTGRIDFTPDELEGATE_H__

#include <list>
#include <string>

#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/Utils.h>
#include <arc/data/DataPointDelegate.h>

namespace ArcDMCGridFTP {

  using namespace Arc;

  /**
   * GridFTP is essentially the FTP protocol with GSI security. This class
   * uses libraries from the Globus Toolkit. It can also be used for regular
   * FTP.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointGridFTPDelegate
    : public DataPointDelegate {
  public:
    DataPointGridFTPDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointGridFTPDelegate();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool RequiresCredentials() const;
    virtual bool SetURL(const Arc::URL&);
    virtual bool WriteOutOfOrder();

  private:
    bool is_secure;
  };

} // namespace ArcDMCGridFTP

#endif // __ARC_DATAPOINTGRIDFTPDELEGATE_H__

