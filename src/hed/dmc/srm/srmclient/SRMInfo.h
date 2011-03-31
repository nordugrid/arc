#ifndef SRM_INFO_H_
#define SRM_INFO_H_

#include <arc/Thread.h>

#include "SRMURL.h"

/**
 * Info about a particular entry in the SRM info file
 */
class SRMFileInfo {
 public:
  std::string host;
  int port;
  enum SRMURL::SRM_URL_VERSION version;
  SRMFileInfo(const std::string& host, int port, const std::string& version);
  SRMFileInfo();
  bool operator==(SRMURL srm_url);
  std::string versionString() const;
};

/**
 * Represents SRM info stored in file. A combination of host and SRM
 * version make a unique entry.
 */
class SRMInfo {
 public:
  SRMInfo(std::string dir);
  bool getSRMFileInfo(SRMFileInfo& srm_file_info);
  void putSRMFileInfo(const SRMFileInfo& srm_file_info);
 private:
  static Arc::SimpleCondition lock;
  // cached info in memory, to avoid constantly reading file
  static std::list<SRMFileInfo> srm_info;
  static Arc::Logger logger;
  std::string srm_info_filename;
};

#endif /*SRM_INFO_H_*/
