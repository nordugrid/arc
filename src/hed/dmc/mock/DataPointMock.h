#ifndef DATAPOINTMOCK_H_
#define DATAPOINTMOCK_H_

#include <arc/data/DataPointDirect.h>

namespace ArcDMCMock {

  using namespace Arc;

  /// Mock data point which does not do anything but sleep for each operation.
  /**
   * If the URL protocol is mock:// then each method returns
   * DataStatus::Success. If it is fail:// then each method returns an error.
   * This plugin is not built by default - to build it the option
   * --enable-mock-dmc must be passed to configure.
   */
  class DataPointMock: public DataPointDirect {
  public:
    DataPointMock(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg);
    virtual ~DataPointMock();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
    virtual bool RequiresCredentials() const { return false; }
  };

} // namespace ArcDMCMock

#endif /* DATAPOINTMOCK_H_ */
