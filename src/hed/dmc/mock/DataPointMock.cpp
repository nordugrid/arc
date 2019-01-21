#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>

#include "DataPointMock.h"

#include <unistd.h>

namespace ArcDMCMock {

  using namespace Arc;

  DataPointMock::DataPointMock(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg)
    : DataPointDirect(url, usercfg, transfer_url, parg) {}

  DataPointMock::~DataPointMock() {}

  DataStatus DataPointMock::StartReading(DataBuffer& buffer) {
    sleep(10);
    buffer.eof_read(true);
    if (url.Protocol() == "fail") return DataStatus::ReadStartError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::StartWriting(DataBuffer& buffer,
                                         DataCallback *) {
    sleep(10);
    buffer.eof_write(true);
    if (url.Protocol() == "fail") return DataStatus::WriteStartError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::StopReading() {
    if (url.Protocol() == "fail") return DataStatus::ReadStopError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::StopWriting() {
    if (url.Protocol() == "fail") return DataStatus::WriteStopError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::Check(bool) {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::CheckError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::Stat(FileInfo&, DataPointInfoType) {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::StatError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::List(std::list<FileInfo>&, DataPointInfoType) {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::ListError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::Remove() {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::DeleteError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::CreateDirectory(bool) {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::CreateDirectoryError;
    return DataStatus::Success;
  }
  DataStatus DataPointMock::Rename(const URL&) {
    sleep(1);
    if (url.Protocol() == "fail") return DataStatus::RenameError;
    return DataStatus::Success;
  }

  Plugin* DataPointMock::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "mock" &&
        ((const URL &)(*dmcarg)).Protocol() != "fail")
      return NULL;
    return new DataPointMock(*dmcarg, *dmcarg, *dmcarg, dmcarg);
  }

} // namespace ArcDMCMock

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "mock", "HED:DMC", "Dummy protocol", 0, &ArcDMCMock::DataPointMock::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
