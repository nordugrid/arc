// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTS3_H__
#define __ARC_DATAPOINTS3_H__

#include <list>
#include <libs3.h>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>

namespace ArcDMCS3 {

using namespace Arc;

/**
 * This class allows access to object stores through the S3 protocol. It uses
 * the environment variables S3_ACCESS_KEY and S3_SECRET_KEY for authentication.
 *
 * This class is a loadable module and cannot be used directly. The DataHandle
 * class loads modules at runtime and should be used instead of this.
 */
class DataPointS3 : public DataPointDirect {
public:
  DataPointS3(const URL &url, const UserConfig &usercfg, PluginArgument *parg);
  virtual ~DataPointS3();
  static Plugin *Instance(PluginArgument *arg);
  virtual DataStatus StartReading(DataBuffer &buffer);
  virtual DataStatus StartWriting(DataBuffer &buffer,
                                  DataCallback *space_cb = NULL);
  virtual DataStatus StopReading();
  virtual DataStatus StopWriting();
  virtual DataStatus Check(bool check_meta);
  virtual DataStatus Stat(FileInfo &file,
                          DataPointInfoType verb = INFO_TYPE_ALL);
  virtual DataStatus List(std::list<FileInfo> &files,
                          DataPointInfoType verb = INFO_TYPE_ALL);
  virtual DataStatus Remove();
  virtual DataStatus CreateDirectory(bool with_parents = false);
  virtual DataStatus Rename(const URL &newurl);
  virtual bool WriteOutOfOrder();
  virtual bool RequiresCredentials() const {
    return false;
  };

  static unsigned long long int offset;

private:
  std::string access_key;
  std::string secret_key;
#if defined(S3_DEFAULT_REGION)
  std::string auth_region;
#endif
  std::string hostname;
  std::string bucket_name;
  std::string key_name;
  S3Protocol protocol;
  S3UriStyle uri_style;
  S3BucketContext bucket_context;
  SimpleCounter transfers_started;

  static void read_file_start(void *arg);
  static void write_file_start(void *arg);
  void read_file();
  void write_file();

  int fd;
  bool reading;
  bool writing;

  static Logger logger;
  static S3Status request_status;
  static char error_details[4096];

  // Callbacks
  static S3Status
  responsePropertiesCallback(const S3ResponseProperties *properties,
                             void *callbackData);

  static S3Status
  headResponsePropertiesCallback(const S3ResponseProperties *properties,
                                 void *callbackData);

  static void responseCompleteCallback(S3Status status,
                                       const S3ErrorDetails *error,
                                       void *callbackData);
  static void getCompleteCallback(S3Status status, const S3ErrorDetails *error,
                                  void *callbackData);

  static void putCompleteCallback(S3Status status, const S3ErrorDetails *error,
                                  void *callbackData);

  static S3Status listBucketCallback(int isTruncated, const char *nextMarker,
                                     int contentsCount,
                                     const S3ListBucketContent *contents,
                                     int commonPrefixesCount,
                                     const char **commonPrefixes,
                                     void *callbackData);

  static S3Status listServiceCallback(const char *ownerId,
                                      const char *ownerDisplayName,
                                      const char *bucketName,
                                      int64_t creationDate, void *callbackData);

  static S3Status getObjectDataCallback(int bufferSize, const char *buffer,
                                        void *callbackData);
};

} // namespace Arc

#endif // __ARC_DATAPOINTS3_H__
