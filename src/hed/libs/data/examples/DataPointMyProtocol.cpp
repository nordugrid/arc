#include <arc/data/DataPointDirect.h>

namespace Arc {

// DMC implementation for my protocol
class DataPointMyProtocol : public DataPointDirect {
 public:
  // Constructor should never be used directly
  DataPointMyProtocol(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
  // Instance is called by the DataPointPluginLoader to get the correct DMC
  // instance. If returns a DataPointMyProtocol if the URL is of the form my://
  // or NULL otherwise.
  static Plugin* Instance(PluginArgument *arg);
  // The following methods from DataPoint must be implemented
  virtual DataStatus Check(bool check_meta);
  virtual DataStatus Remove();
  virtual DataStatus CreateDirectory(bool with_parents=false) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
  virtual DataStatus Stat(FileInfo& file, DataPoint::DataPointInfoType verb);
  virtual DataStatus List(std::list<FileInfo>& file, DataPoint::DataPointInfoType verb);
  virtual DataStatus Rename(const URL& newurl) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
  virtual DataStatus StartReading(DataBuffer& buffer);
  virtual DataStatus StartWriting(DataBuffer& buffer,
                                  DataCallback *space_cb = NULL);
  virtual DataStatus StopReading();
  virtual DataStatus StopWriting();
};

DataPointMyProtocol::DataPointMyProtocol(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
 : DataPointDirect(url, usercfg, parg) {}

DataStatus DataPointMyProtocol::Check(bool check_meta) { return DataStatus::Success; }
DataStatus DataPointMyProtocol::Remove() { return DataStatus::Success; }
DataStatus DataPointMyProtocol::Stat(FileInfo& file,
                                     DataPoint::DataPointInfoType verb) { return DataStatus::Success; }
DataStatus DataPointMyProtocol::List(std::list<FileInfo>& file,
                                     DataPoint::DataPointInfoType verb) { return DataStatus::Success; }
DataStatus DataPointMyProtocol::StartReading(DataBuffer& buffer) { return DataStatus::Success; }
DataStatus DataPointMyProtocol::StartWriting(DataBuffer& buffer,
                                             DataCallback *space_cb) { return DataStatus::Success; }
DataStatus DataPointMyProtocol::StopReading() { return DataStatus::Success; }
DataStatus DataPointMyProtocol::StopWriting() { return DataStatus::Success; }

Plugin* DataPointMyProtocol::Instance(PluginArgument *arg) {
  DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
  if (!dmcarg)
    return NULL;
  if (((const URL &)(*dmcarg)).Protocol() != "my")
    return NULL;
  return new DataPointMyProtocol(*dmcarg, *dmcarg, dmcarg);
}

} // namespace Arc

// Add this plugin to the plugin descriptor table
extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "my", "HED:DMC", "My protocol", 0, &Arc::DataPointMyProtocol::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
