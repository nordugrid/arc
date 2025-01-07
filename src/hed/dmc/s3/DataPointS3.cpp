// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/FileUtils.h>
#include <arc/FileAccess.h>
#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
//#include <arc/CheckSum.h>
#include <arc/Utils.h>

#include "DataPointS3.h"

#if defined(HAVE_S3_TIMEOUT)
#define S3_TIMEOUTMS 0
#endif

// assume these changes introduced the same time as authRegion
// TODO: find better way to distinguish libs3 versions
#if defined(S3_DEFAULT_REGION)
#define S3_SECURITY_TOKEN
#define S3_SERVER_SIDE_ENCRYPTION
#endif
namespace ArcDMCS3 {

using namespace Arc;

// Static class variables
Logger DataPointS3::logger(Logger::getRootLogger(), "DataPoint.S3");

S3Status DataPointS3::request_status = S3Status(0);

unsigned long long int DataPointS3::offset = 0;

char ArcDMCS3::DataPointS3::error_details[4096] = { 0 };

S3Status
DataPointS3::responsePropertiesCallback(const S3ResponseProperties *properties,
                                        void *callbackData) {
  return S3StatusOK;
}

void DataPointS3::getCompleteCallback(S3Status status,
                                      const S3ErrorDetails *error,
                                      void *callbackData) {

  request_status = status;
  if (status == S3StatusOK) {
    DataBuffer *buf = (DataBuffer *)callbackData;
    buf->eof_read(true);
  } else {
    int len = 0;
    if (error && error->message) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Message: %s;", error->message);
    }
    if (error && error->resource) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Resource: %s;", error->resource);
    }
    if (error && error->furtherDetails) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Further Details: %s;", error->furtherDetails);
    }
    if (error && error->extraDetailsCount) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len, "%s",
                      "Extra Details:");
      int i;
      for (i = 0; i < error->extraDetailsCount; i++) {
        len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                        " %s: %s;", error->extraDetails[i].name,
                        error->extraDetails[i].value);
      }
    }
  }
}

void DataPointS3::putCompleteCallback(S3Status status,
                                      const S3ErrorDetails *error,
                                      void *callbackData) {

  request_status = status;
  if (status == S3StatusOK) {
    DataBuffer *buf = (DataBuffer *)callbackData;
    buf->eof_write(true);
  } else {

    int len = 0;
    if (error && error->message) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Message: %s;", error->message);
    }
    if (error && error->resource) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Resource: %s;", error->resource);
    }
    if (error && error->furtherDetails) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      "Further Details: %s;", error->furtherDetails);
    }
    if (error && error->extraDetailsCount) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len, "%s",
                      "Extra Details:");
      int i;
      for (i = 0; i < error->extraDetailsCount; i++) {
        len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                        " %s: %s;", error->extraDetails[i].name,
                        error->extraDetails[i].value);
      }
    }
  }
}

void DataPointS3::responseCompleteCallback(S3Status status,
                                           const S3ErrorDetails *error,
                                           void *callbackData) {

  request_status = status;

  int len = 0;
  if (error && error->message) {
    len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                    "Message: %s;", error->message);
  }
  if (error && error->resource) {
    len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                    "Resource: %s;", error->resource);
  }
  if (error && error->furtherDetails) {
    len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                    "Further Details: %s;", error->furtherDetails);
  }
  if (error && error->extraDetailsCount) {
    len += snprintf(&(error_details[len]), sizeof(error_details) - len, "%s",
                    "Extra Details:");
    int i;
    for (i = 0; i < error->extraDetailsCount; i++) {
      len += snprintf(&(error_details[len]), sizeof(error_details) - len,
                      " %s: %s;", error->extraDetails[i].name,
                      error->extraDetails[i].value);
    }
  }
}

// get object ----------------------------------------------------------------
S3Status DataPointS3::getObjectDataCallback(int bufferSize, const char *buffer,
                                            void *callbackData) {

  DataBuffer *buf = (DataBuffer *)callbackData;

  /* 1. claim buffer */
  int h;
  unsigned int l;
  if (!buf->for_read(h, l, true)) {
    /* failed to get buffer - must be error or request to exit */
    buf->error_read(true);
    return S3StatusOK;
  }

  /* 2. read */
  memcpy((*(buf))[h], buffer, bufferSize);

  /* 3. announce */
  buf->is_read(h, bufferSize, DataPointS3::offset);

  DataPointS3::offset += bufferSize;

  return S3StatusOK;
}

int DataPointS3::putObjectDataCallback(int bufferSize, char *buffer,
                                 void *callbackData) {

  DataBuffer *buf = (DataBuffer *)callbackData;

  /* 1. claim buffer */
  int h;
  unsigned int l;
  unsigned long long int p;
  if (!buf->for_write(h, l, p, true)) {
    // no more data from the buffer, did the other side finished?
    buf->eof_write(true);
    return 0;
  }

  /* 2. write */
  int toCopy = ((l > (unsigned)bufferSize) ? (unsigned)bufferSize : l);
  memcpy(buffer, (*(buf))[h], toCopy);

  /* 3. announce */
  buf->is_written(h);

  return toCopy;
}

DataPointS3::DataPointS3(const URL &url, const UserConfig &usercfg,
                         PluginArgument *parg)
    : DataPointDirect(url, usercfg, parg), fd(-1), reading(false),
      writing(false) {
  // S3 endpoint
  hostname = url.Host();
  // Check scheme
  if (url.Protocol() == "s3+https") {
    protocol = S3ProtocolHTTPS;
    if (url.Port() != 443) {
      hostname = std::string(url.Host() + ":" + tostring(url.Port()));
    }
  } else {
    protocol = S3ProtocolHTTP;
    if (url.Port() != 80) {
      hostname = std::string(url.Host() + ":" + tostring(url.Port()));
    }
  }
  // S3 DMC uses the path-style
  uri_style = S3UriStylePath;
  // S3 credentials (url options or env variables)
  access_key = url.Option("s3_access_key", Arc::GetEnv("S3_ACCESS_KEY"));
  secret_key = url.Option("s3_secret_key", Arc::GetEnv("S3_SECRET_KEY"));
#if defined(S3_DEFAULT_REGION)
  auth_region = url.Option("s3_auth_region", Arc::GetEnv("S3_AUTH_REGION"));
#endif

  // Extract bucket
  bucket_name = url.Path();
  // Remove leading slash
  if (bucket_name.find('/') == 0) {
    bucket_name = bucket_name.substr(1);
  }

  // Remove trailing slash
  if (bucket_name.rfind('/') == bucket_name.length() - 1) {
    bucket_name = bucket_name.substr(0, bucket_name.length() - 1);
  }

  // extract key
  std::size_t found = bucket_name.find('/');
  if (found != std::string::npos) {
    key_name = bucket_name.substr(found + 1, bucket_name.length() - 1);
    bucket_name = bucket_name.substr(0, found);
  }

  // S3_validate_bucket_name

  // if / in key_name or bucket_name then Invalid bucket/key name
  if (bucket_name.find('/') || key_name.find("/")) {
  }
  
  logger.msg(DEBUG, "Initializing S3 connection to %s", hostname.c_str());

  S3Status init_status;
  if ((init_status = S3_initialize("s3", S3_INIT_ALL, hostname.c_str()))
        != S3StatusOK) {
    logger.msg(ERROR, "Failed to initialize S3 to %s: %s", hostname.c_str(),
                      S3_get_status_name(init_status));
  }
  bufsize = 16384;
}

DataPointS3::~DataPointS3() { S3_deinitialize(); }

Plugin *DataPointS3::Instance(PluginArgument *arg) {
  DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument *>(arg);
  if (!dmcarg)
    return NULL;

  if (((const URL &)(*dmcarg)).Protocol() != "s3" &&
      ((const URL &)(*dmcarg)).Protocol() != "s3+http" &&
      ((const URL &)(*dmcarg)).Protocol() != "s3+https")
    return NULL;

  return new DataPointS3(*dmcarg, *dmcarg, dmcarg);
}

DataStatus DataPointS3::Check(bool check_meta) { return DataStatus::Success; }

S3Status DataPointS3::headResponsePropertiesCallback(
    const S3ResponseProperties *properties, void *callbackData) {

  FileInfo *file = (FileInfo *)callbackData;

  file->SetType(FileInfo::file_type_file);
  file->SetSize(properties->contentLength);
  file->SetModified(properties->lastModified);

  return S3StatusOK;
}

DataStatus DataPointS3::Stat(FileInfo &file, DataPointInfoType verb) {

  if (!bucket_name.empty() && !key_name.empty()) {
    S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                      protocol,           uri_style,
                                      access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

    S3ResponseHandler responseHandler = { &headResponsePropertiesCallback,
                                          &responseCompleteCallback };
    file.SetName(key_name);

#if defined(S3_TIMEOUTMS)
    S3_head_object(&bucketContext, key_name.c_str(), NULL, S3_TIMEOUTMS, &responseHandler,
#else
    S3_head_object(&bucketContext, key_name.c_str(), NULL, &responseHandler,
#endif
                   &file);

    if (request_status == S3StatusOK) {
      return DataStatus::Success;
    }
    return DataStatus(DataStatus::StatError,
                      S3_get_status_name(request_status));
  }
  return DataStatus::StatError;
}

S3Status DataPointS3::listBucketCallback(
    int isTruncated, const char *nextMarker, int contentsCount,
    const S3ListBucketContent *contents, int commonPrefixesCount,
    const char **commonPrefixes, void *callbackData) {

  std::list<FileInfo> *files = (std::list<FileInfo> *)callbackData;

  for (int i = 0; i < contentsCount; i++) {
    const S3ListBucketContent *content = &(contents[i]);
    time_t t = (time_t)content->lastModified;

    FileInfo file = FileInfo(content->key);
    file.SetType(FileInfo::file_type_file);
    file.SetSize((unsigned long long)content->size);
    file.SetModified(t);

    file.SetMetaData("group", content->ownerDisplayName);
    file.SetMetaData("owner", content->ownerDisplayName);

    std::list<FileInfo>::iterator f = files->insert(files->end(), file);
  }

  return S3StatusOK;
}

S3Status DataPointS3::listServiceCallback(const char *ownerId,
                                          const char *ownerDisplayName,
                                          const char *bucketName,
                                          int64_t creationDate,
                                          void *callbackData) {

  std::list<FileInfo> *files = (std::list<FileInfo> *)callbackData;

  FileInfo file = FileInfo(bucketName);
  file.SetType(FileInfo::file_type_dir);
  file.SetMetaData("group", ownerDisplayName);
  file.SetMetaData("owner", ownerDisplayName);
  file.SetModified(creationDate);

  std::list<FileInfo>::iterator f = files->insert(files->end(), file);

  return S3StatusOK;
}

DataStatus DataPointS3::List(std::list<FileInfo> &files,
                             DataPointInfoType verb) {

  if (!bucket_name.empty() && !key_name.empty()) {
    FileInfo file(key_name);

    S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                      protocol,           uri_style,
                                      access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

    S3ResponseHandler responseHandler = { &headResponsePropertiesCallback,
                                          &responseCompleteCallback };

#if defined(S3_TIMEOUTMS)
    S3_head_object(&bucketContext, key_name.c_str(), NULL, S3_TIMEOUTMS, &responseHandler,
#else
    S3_head_object(&bucketContext, key_name.c_str(), NULL, &responseHandler,
#endif
                   &file);

    if (request_status == S3StatusOK) {

      std::list<FileInfo>::iterator f = files.insert(files.end(), file);

      return DataStatus::Success;
    }
    return DataStatus(DataStatus::StatError,
                      S3_get_status_name(request_status));
  } else if (!bucket_name.empty()) {

    S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                      protocol,           uri_style,
                                      access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

    S3ListBucketHandler listBucketHandler = { { &responsePropertiesCallback,
                                                &responseCompleteCallback },
                                              &listBucketCallback };

    S3_list_bucket(&bucketContext, NULL, NULL, NULL, 0, NULL,
#if defined(S3_TIMEOUTMS)
                   S3_TIMEOUTMS,
#endif
                   &listBucketHandler, &files);

  } else {

    S3ListServiceHandler listServiceHandler = { { &responsePropertiesCallback,
                                                  &responseCompleteCallback },
                                                &listServiceCallback };

    S3_list_service(protocol, access_key.c_str(), secret_key.c_str(), 
#if defined(S3_SECURITY_TOKEN)
                    0,  // securityToken
#endif
                    NULL, // hostName
#if defined(S3_DEFAULT_REGION)
                    auth_region.c_str(), // authRegion
#endif
                    NULL, // requestContext
#if defined(S3_TIMEOUTMS)
                    S3_TIMEOUTMS, //timeoutMs
#endif
                    &listServiceHandler, &files);
  }

  if (request_status == S3StatusOK) {
    return DataStatus::Success;
  }
  logger.msg(ERROR, "Failed to read object %s: %s; %s", url.Path(),
             S3_get_status_name(request_status), error_details);
  return DataStatus(DataStatus::ListError, S3_get_status_name(request_status));
}

DataStatus DataPointS3::Remove() {

  if (key_name.empty()) {
    S3ResponseHandler responseHandler = { &responsePropertiesCallback,
                                          &responseCompleteCallback };

    S3_delete_bucket(protocol, uri_style, access_key.c_str(),secret_key.c_str(),
#if defined(S3_SECURITY_TOKEN)
                    0,  // securityToken
#endif
                    0, // hostName
                    bucket_name.c_str(), //bucketName
#if defined(S3_DEFAULT_REGION)
                    auth_region.c_str(), // authRegion
#endif
                    NULL, // requestContext
#if defined(S3_TIMEOUTMS)
                    S3_TIMEOUTMS, //timeoutMs
#endif
                    &responseHandler, 0);
  } else {
    S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                      protocol,           uri_style,
                                      access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

    S3ResponseHandler responseHandler = { 0, &responseCompleteCallback };

#if defined(S3_TIMEOUTMS)
    S3_delete_object(&bucketContext, key_name.c_str(), NULL, S3_TIMEOUTMS, &responseHandler, 0);
#else
    S3_delete_object(&bucketContext, key_name.c_str(), 0, &responseHandler, 0);
#endif
  }

  if (request_status == S3StatusOK) {
    return DataStatus::Success;
  }

  return DataStatus(DataStatus::DeleteError, EINVAL,
                    S3_get_status_name(request_status));
}

DataStatus DataPointS3::Rename(const URL &newurl) {
  return DataStatus(DataStatus::RenameError, ENOTSUP,
                    "Renaming in S3 is not supported");
}

DataStatus DataPointS3::CreateDirectory(bool with_parents) {

  if (!key_name.empty()) {
    return DataStatus(DataStatus::CreateDirectoryError, EINVAL,
                      "key should not be given");
  }

  S3ResponseHandler responseHandler = { &responsePropertiesCallback,
                                        &responseCompleteCallback };

  S3CannedAcl cannedAcl = S3CannedAclPrivate;
  S3_create_bucket(protocol, access_key.c_str(), secret_key.c_str(), 
#if defined(S3_SECURITY_TOKEN)
                    0,  // securityToken
#endif
                    0, // hostName
                    bucket_name.c_str(), //bucketName
#if defined(S3_DEFAULT_REGION)
                    auth_region.c_str(), // authRegion
#endif
                    cannedAcl, 0, 0, // cannedAcl,locationConstraint,requestContext
#if defined(S3_TIMEOUTMS)
                    S3_TIMEOUTMS, //timeoutMs
#endif
                    &responseHandler, 0);

  if (request_status == S3StatusOK) {
    return DataStatus::Success;
  }

  return DataStatus(DataStatus::CreateDirectoryError, EINVAL,
                    S3_get_status_name(request_status));
}

void DataPointS3::read_file_start(void *arg) {
  ((DataPointS3 *)arg)->read_file();
}

void DataPointS3::read_file() {

  S3GetObjectHandler getObjectHandler = { { &responsePropertiesCallback,
                                            &DataPointS3::getCompleteCallback },
                                          &DataPointS3::getObjectDataCallback };

  S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                    protocol,           uri_style,
                                    access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

  uint64_t startByte = 0, byteCount = 0;
  S3_get_object(&bucketContext, key_name.c_str(), 0, startByte, byteCount, 0,
#if defined(S3_TIMEOUTMS)
                S3_TIMEOUTMS,
#endif
                &getObjectHandler, buffer);

  if (request_status != S3StatusOK) {
    logger.msg(ERROR, "Failed to read object %s: %s; %s", url.Path(),
               S3_get_status_name(request_status), error_details);
    buffer->error_read(true);
  }
}

DataStatus DataPointS3::StartReading(DataBuffer &buf) {
  if (reading)
    return DataStatus::IsReadingError;
  if (writing)
    return DataStatus::IsWritingError;
  reading = true;

  buffer = &buf;
  // create thread to maintain reading
  if (!CreateThreadFunction(&DataPointS3::read_file_start, this,
                            &transfers_started)) {
    reading = false;
    buffer = NULL;
    return DataStatus::ReadStartError;
  }

  return DataStatus::Success;
}

DataStatus DataPointS3::StopReading() {

  transfers_started.wait();
  return DataStatus::Success;
}

void DataPointS3::write_file_start(void *arg) {
  ((DataPointS3 *)arg)->write_file();
}

void DataPointS3::write_file() {

  S3BucketContext bucketContext = { 0,                  bucket_name.c_str(),
                                    protocol,           uri_style,
                                    access_key.c_str(), secret_key.c_str()
#if defined(S3_SECURITY_TOKEN)
                                      , NULL
#endif
#if defined(S3_DEFAULT_REGION)
                                      , auth_region.c_str()
#endif
                                      };

  S3PutObjectHandler putObjectHandler = { { &responsePropertiesCallback,
                                            &DataPointS3::putCompleteCallback },
                                          &DataPointS3::putObjectDataCallback };

  const char *cacheControl = 0, *contentType = 0, *md5 = 0;
  const char *contentDispositionFilename = 0, *contentEncoding = 0;
  int64_t expires = -1;
  S3CannedAcl cannedAcl = S3CannedAclPrivate;
  int metaPropertiesCount = 0;
  S3NameValue metaProperties[S3_MAX_METADATA_COUNT];
  char useServerSideEncryption = 0;

  S3PutProperties putProperties = { contentType,     md5,
                                    cacheControl,    contentDispositionFilename,
                                    contentEncoding, expires,
                                    cannedAcl,       metaPropertiesCount,
                                    metaProperties
#if defined(S3_SERVER_SIDE_ENCRYPTION)
                                    ,  useServerSideEncryption 
#endif
                                    };

  S3_put_object(&bucketContext, key_name.c_str(), size, &putProperties, NULL,
#if defined(S3_TIMEOUTMS)
                S3_TIMEOUTMS,
#endif
                &putObjectHandler, buffer);

  if (request_status != S3StatusOK) {
    logger.msg(ERROR, "Failed to write object %s: %s; %s", url.Path(),
               S3_get_status_name(request_status), error_details);

    buffer->error_write(true);
  }
}

DataStatus DataPointS3::StartWriting(DataBuffer &buf, DataCallback *space_cb) {
  if (reading)

    return DataStatus::IsReadingError;
  if (writing)
    return DataStatus::IsWritingError;
  writing = true;

  /* Check if size for source is defined */
  if (!CheckSize()) {
    return DataStatus(DataStatus::WriteStartError,
                      "Size of the source file missing. S3 needs to know it.");
  }

  /* try to open */
  buffer = &buf;
  buffer->set(NULL, 16384, 3);
  buffer->speed.reset();
  buffer->speed.hold(false);
  /* create thread to maintain writing */
  if (!CreateThreadFunction(&DataPointS3::write_file_start, this,
                            &transfers_started)) {
    buffer->error_write(true);
    buffer->eof_write(true);
    writing = false;
    return DataStatus(DataStatus::WriteStartError,
                      "Failed to create new thread");
  }

  return DataStatus::Success;
}

DataStatus DataPointS3::StopWriting() {
  writing = false;
  transfers_started.wait(); /* wait till writing thread exited */
  buffer = NULL;
  return DataStatus::Success;
}

bool DataPointS3::WriteOutOfOrder() const { return false; }

} // namespace Arc

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "s3", "HED:DMC", "Amazon S3 Store", 0, &ArcDMCS3::DataPointS3::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
