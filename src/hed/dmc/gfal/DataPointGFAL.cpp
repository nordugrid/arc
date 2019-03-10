// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glibmm.h>

#include <gfal_api.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
#include <arc/CheckSum.h>
#include <arc/FileUtils.h>
#include <arc/FileAccess.h>
#include <arc/Utils.h>

#include "DataPointGFAL.h"
#include "GFALTransfer3rdParty.h"
#include "GFALUtils.h"

namespace ArcDMCGFAL {

  using namespace Arc;

  /// Class for locking environment while calling gfal functions.
  class GFALEnvLocker: public CertEnvLocker {
  public:
    static Logger logger;
    GFALEnvLocker(const UserConfig& usercfg, const std::string& lfc_host): CertEnvLocker(usercfg) {
      EnvLockUnwrap(false);
      // if root, we have to set X509_USER_CERT and X509_USER_KEY to
      // X509_USER_PROXY to force GFAL to use the proxy. If they are undefined
      // it uses the host cert and key.
      if (getuid() == 0 && !GetEnv("X509_USER_PROXY").empty()) {
        SetEnv("X509_USER_KEY", GetEnv("X509_USER_PROXY"), true);
        SetEnv("X509_USER_CERT", GetEnv("X509_USER_PROXY"), true);
      }
      logger.msg(DEBUG, "Using proxy %s", GetEnv("X509_USER_PROXY"));
      logger.msg(DEBUG, "Using key %s", GetEnv("X509_USER_KEY"));
      logger.msg(DEBUG, "Using cert %s", GetEnv("X509_USER_CERT"));

      if (!lfc_host.empty()) {
        // set LFC retry env variables (don't overwrite if set already)
        // connection timeout
        SetEnv("LFC_CONNTIMEOUT", "30", false);
        // number of retries
        SetEnv("LFC_CONRETRY", "1", false);
        // interval between retries
        SetEnv("LFC_CONRETRYINT", "10", false);

        // set host name env var
        SetEnv("LFC_HOST", lfc_host);
      }

      EnvLockWrap(false);
    }
  };

  Logger GFALEnvLocker::logger(Logger::getRootLogger(), "GFALEnvLocker");

  Logger DataPointGFAL::logger(Logger::getRootLogger(), "DataPoint.GFAL");

  DataPointGFAL::DataPointGFAL(const URL& u, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(u, usercfg, parg), fd(-1), reading(false), writing(false) {
      LogLevel loglevel = logger.getThreshold();
      if (loglevel == DEBUG)
        gfal2_log_set_level (G_LOG_LEVEL_DEBUG);
      if (loglevel == VERBOSE)
        gfal2_log_set_level (G_LOG_LEVEL_INFO);
      if (url.Protocol() == "lfc")
        lfc_host = url.Host();
  }

  DataPointGFAL::~DataPointGFAL() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointGFAL::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "rfio" &&
        ((const URL &)(*dmcarg)).Protocol() != "dcap" &&
        ((const URL &)(*dmcarg)).Protocol() != "gsidcap" &&
        ((const URL &)(*dmcarg)).Protocol() != "lfc" &&
        // gfal protocol is used in 3rd party transfer to load this DMC
        ((const URL &)(*dmcarg)).Protocol() != "gfal")
      return NULL;
    return new DataPointGFAL(*dmcarg, *dmcarg, dmcarg);
  }

  DataStatus DataPointGFAL::Resolve(bool source) {
    // Here we just deal with getting locations from destination
    if (source || (url.Protocol() != "lfn" && url.Protocol() != "guid")) return DataStatus::Success;

    if (url.Locations().size() == 0 && locations.empty()) {
      logger.msg(ERROR, "Locations are missing in destination LFC URL");
      return DataStatus(DataStatus::WriteResolveError, EINVAL, "No locations specified");
    }

    for (std::list<URLLocation>::const_iterator u = url.Locations().begin(); u != url.Locations().end(); ++u) {
      if (AddLocation(*u, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError) {
        logger.msg(WARNING, "Duplicate replica found in LFC: %s", u->plainstr());
      } else {
        logger.msg(VERBOSE, "Adding location: %s - %s", url.ConnectionURL(), u->plainstr());
      }
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGFAL::AddLocation(const URL& url,
                                        const std::string& meta) {
    logger.msg(DEBUG, "Add location: url: %s", url.str());
    logger.msg(DEBUG, "Add location: metadata: %s", meta);
    for (std::list<URLLocation>::iterator i = locations.begin();
         i != locations.end(); ++i)
      if ((i->Name() == meta) && (url == (*i)))
        return DataStatus::LocationAlreadyExistsError;
    locations.push_back(URLLocation(url, meta));
    return DataStatus::Success;
  }

  DataStatus DataPointGFAL::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    
    // Open the file
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      fd = gfal_open(GFALUtils::GFALURL(url).c_str(), O_RDONLY, 0);
    }
    if (fd < 0) {
      logger.msg(VERBOSE, "gfal_open failed: %s", StrError(errno));
      int error_no = GFALUtils::HandleGFALError(logger);
      reading = false;
      return DataStatus(DataStatus::ReadStartError, error_no);
    }
    
    // Remember the DataBuffer we got: the separate reading thread will use it
    buffer = &buf;
    // StopReading will wait for this condition,
    // which will be signalled by the separate reading thread
    // Create the separate reading thread
    if (!CreateThreadFunction(&DataPointGFAL::read_file_start, this, &transfer_condition)) {
      if (fd != -1 && gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      reading = false;
      return DataStatus(DataStatus::ReadStartError, "Failed to create reading thread");
    }
    return DataStatus::Success;
  }
  
  void DataPointGFAL::read_file_start(void *object) {
    ((DataPointGFAL*)object)->read_file();
  }
  
  void DataPointGFAL::read_file() {
    int handle;
    unsigned int length;
    unsigned long long int offset = 0;
    ssize_t bytes_read = 0;
    for (;;) {
      // Ask the DataBuffer for a buffer to read into
      if (!buffer->for_read(handle, length, true)) {
        buffer->error_read(true);
        break;
      }

      // Read into the buffer
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        bytes_read = gfal_read(fd, (*(buffer))[handle], length);
      }

      // If there was an error
      if (bytes_read < 0) {
        logger.msg(VERBOSE, "gfal_read failed: %s", StrError(errno));
        GFALUtils::HandleGFALError(logger);
        buffer->error_read(true);
        break;
      }
      
      // If there was no more to read
      if (bytes_read == 0) {
        buffer->is_read(handle, 0, offset);
        break;
      }
      
      // Tell the DataBuffer that we read something into it
      buffer->is_read(handle, bytes_read, offset);
      // Keep track of where we are in the file
      offset += bytes_read;
    }
    // We got out of the loop, which means we read the whole file
    // or there was an error, either case the reading is finished
    buffer->eof_read(true);
    // Close the file
    if (fd != -1) {
      int r;
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        r = gfal_close(fd);
      }
      if (r < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      fd = -1;
    }
  }
  
  DataStatus DataPointGFAL::StopReading() {
    if (!reading) return DataStatus(DataStatus::ReadStopError, EARCLOGIC, "Not reading");
    reading = false;
    if (!buffer) return DataStatus(DataStatus::ReadStopError, EARCLOGIC, "Not reading");
    // If the reading is not finished yet trigger reading error
    if (!buffer->eof_read()) buffer->error_read(true);

    // Wait for the reading thread to finish
    logger.msg(DEBUG, "StopReading starts waiting for transfer_condition.");
    transfer_condition.wait();
    logger.msg(DEBUG, "StopReading finished waiting for transfer_condition.");

    // Close the file if not already done
    if (fd != -1) {
      int r;
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        r = gfal_close(fd);
      }
      if (r < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      fd = -1;
    }
    // If there was an error (maybe we triggered it)
    if (buffer->error_read()) {
      buffer = NULL;
      return DataStatus::ReadError;
    }
    // If there was no error (the reading already finished)
    buffer = NULL;
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::StartWriting(DataBuffer& buf, DataCallback *space_cb) {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC);
    writing = true;

    // if index service (eg LFC) then set the replica with extended attrs
    if (url.Protocol() == "lfn" || url.Protocol() == "guid") {
      if (locations.empty()) {
        logger.msg(ERROR, "No locations defined for %s", url.str());
        writing = false;
        return DataStatus(DataStatus::WriteStartError, EINVAL, "No locations defined");
      }
      // choose first location
      std::string location(locations.begin()->plainstr());
      if (gfal_setxattr(GFALUtils::GFALURL(url).c_str(), "user.replicas", location.c_str(), location.length(), 0) != 0) {
        logger.msg(VERBOSE, "Failed to set LFC replicas: %s", StrError(gfal_posix_code_error()));
        int error_no = GFALUtils::HandleGFALError(logger);
        writing = false;
        return DataStatus(DataStatus::WriteStartError, error_no, "Failed to set LFC replicas");
      }
    }
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      // Open the file
      fd = gfal_open(GFALUtils::GFALURL(url).c_str(), O_WRONLY | O_CREAT, 0600);
    }
    if (fd < 0) {
      // If no entry try to create parent directories
      if (errno == ENOENT) {
        URL parent_url = URL(url.plainstr());
        // For SRM the path can be given as SFN HTTP option
        if ((url.Protocol() == "srm" && !url.HTTPOption("SFN").empty())) {
          parent_url.AddHTTPOption("SFN", Glib::path_get_dirname(url.HTTPOption("SFN")), true);
        } else {
          parent_url.ChangePath(Glib::path_get_dirname(url.Path()));
        }

        {
          GFALEnvLocker gfal_lock(usercfg, lfc_host);
          // gfal_mkdir is always recursive
          if (gfal_mkdir(GFALUtils::GFALURL(parent_url).c_str(), 0700) != 0 && gfal_posix_code_error() != EEXIST) {
            logger.msg(INFO, "gfal_mkdir failed (%s), trying to write anyway", StrError(gfal_posix_code_error()));
          }
          fd = gfal_open(GFALUtils::GFALURL(url).c_str(), O_WRONLY | O_CREAT, 0600);
        }
      }
      if (fd < 0) {
        logger.msg(VERBOSE, "gfal_open failed: %s", StrError(gfal_posix_code_error()));
        int error_no = GFALUtils::HandleGFALError(logger);
        writing = false;
        return DataStatus(DataStatus::WriteStartError, error_no);
      }
    }
    
    // Remember the DataBuffer we got, the separate writing thread will use it
    buffer = &buf;
    // StopWriting will wait for this condition,
    // which will be signalled by the separate writing thread
    // Create the separate writing thread
    if (!CreateThreadFunction(&DataPointGFAL::write_file_start, this, &transfer_condition)) {
      if (fd != -1 && gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create writing thread");
    }    
    return DataStatus::Success;
  }
  
  void DataPointGFAL::write_file_start(void *object) {
    ((DataPointGFAL*)object)->write_file();
  }
  
  void DataPointGFAL::write_file() {
    int handle;
    unsigned int length;
    unsigned long long int position;
    unsigned long long int offset = 0;
    ssize_t bytes_written = 0;
    unsigned int chunk_offset;
    
    for (;;) {
      // Ask the DataBuffer for a buffer with data to write,
      // and the length and position where to write to
      if (!buffer->for_write(handle, length, position, true)) {
        // no more data from the buffer, did the other side finished?
        if (!buffer->eof_read()) {
          // the other side hasn't finished yet, must be an error
          buffer->error_write(true);
        }
        break;
      }      

      // if the buffer gives different position than we are currently in the
      // destination, then we have to seek there
      if (position != offset) {
        logger.msg(DEBUG, "DataPointGFAL::write_file got position %d and offset %d, has to seek", position, offset);
        {
          GFALEnvLocker gfal_lock(usercfg, lfc_host);
          gfal_lseek(fd, position, SEEK_SET);
        }
        offset = position;
      }
      
      // we want to write the chunk we got from the buffer,
      // but we may not be able to write it in one shot
      chunk_offset = 0;
      while (chunk_offset < length) {
        {
          GFALEnvLocker gfal_lock(usercfg, lfc_host);
          bytes_written = gfal_write(fd, (*(buffer))[handle] + chunk_offset, length - chunk_offset);
        }
        if (bytes_written < 0) break; // there was an error
        // calculate how far we got into to the chunk
        chunk_offset += bytes_written;
        // if the new chunk_offset is still less then the length of the chunk,
        // we have to continue writing
      }

      // we finished with writing (even if there was an error)
      buffer->is_written(handle);
      offset += length;

      // if there was an error during writing
      if (bytes_written < 0) {
        logger.msg(VERBOSE, "gfal_write failed: %s", StrError(gfal_posix_code_error()));
        GFALUtils::HandleGFALError(logger);
        buffer->error_write(true);
        break;
      }
    }
    buffer->eof_write(true);
    // Close the file
    if (fd != -1) {
      int r;
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        r = gfal_close(fd);
      }
      if (r < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      fd = -1;
    }
  }
    
  DataStatus DataPointGFAL::StopWriting() {
    if (!writing) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");
    writing = false;
    if (!buffer) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");
    
    // If the writing is not finished, trigger writing error
    if (!buffer->eof_write()) buffer->error_write(true);

    // Wait until the writing thread finishes
    logger.msg(DEBUG, "StopWriting starts waiting for transfer_condition.");
    transfer_condition.wait();
    logger.msg(DEBUG, "StopWriting finished waiting for transfer_condition.");

    // Close the file if not done already
    if (fd != -1) {
      int r;
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        r = gfal_close(fd);
      }
      if (r < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(gfal_posix_code_error()));
      }
      fd = -1;
    }
    // If there was an error (maybe we triggered it)
    if (buffer->error_write()) {
      buffer = NULL;
      return DataStatus::WriteError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }  
  
  DataStatus DataPointGFAL::do_stat(const URL& stat_url, FileInfo& file, DataPointInfoType verb) {
    struct stat st;
    int res;
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      res = gfal_stat(GFALUtils::GFALURL(stat_url).c_str(), &st);
    }
    if (res < 0) {
      logger.msg(VERBOSE, "gfal_stat failed: %s", StrError(gfal_posix_code_error()));
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::StatError, error_no);
    }

    if(S_ISREG(st.st_mode)) {
      file.SetType(FileInfo::file_type_file);
    } else if(S_ISDIR(st.st_mode)) {
      file.SetType(FileInfo::file_type_dir);
    } else {
      file.SetType(FileInfo::file_type_unknown);
    }

    std::string path = stat_url.Path();
    // For SRM the path can be given as SFN HTTP Option
    if ((stat_url.Protocol() == "srm" && !stat_url.HTTPOption("SFN").empty())) path = stat_url.HTTPOption("SFN");

    std::string name = Glib::path_get_basename(path);
    file.SetName(name);

    file.SetSize(st.st_size);
    file.SetModified(st.st_mtime);
    file.SetMetaData("atime", (Time(st.st_atime)).str());
    file.SetMetaData("ctime", (Time(st.st_ctime)).str());

    std::string perms;
    if (st.st_mode & S_IRUSR) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWUSR) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXUSR) perms += 'x'; else perms += '-';
    if (st.st_mode & S_IRGRP) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWGRP) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXGRP) perms += 'x'; else perms += '-';
    if (st.st_mode & S_IROTH) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWOTH) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXOTH) perms += 'x'; else perms += '-';
    file.SetMetaData("accessperm", perms);

    if (verb & INFO_TYPE_STRUCT) {
      char replicas[65536];
      ssize_t r;
      {
        GFALEnvLocker gfal_lock(usercfg, lfc_host);
        r = gfal_getxattr(GFALUtils::GFALURL(stat_url).c_str(), "user.replicas", replicas, sizeof(replicas));
      }
      if (r < 0) {
        logger.msg(VERBOSE, "gfal_listxattr failed, no replica information can be obtained: %s", StrError(gfal_posix_code_error()));
      } else {
        std::vector<std::string> reps;
        tokenize(replicas, reps, "\n");
        for (std::vector<std::string>::const_iterator u = reps.begin(); u != reps.end(); ++u) {
          file.AddURL(URL(*u));
        }
      }
    }
    return DataStatus::Success;    
  }

  DataStatus DataPointGFAL::Check(bool check_meta) {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC);
    
    FileInfo file;
    DataStatus status_from_stat = do_stat(url, file, (DataPointInfoType)(INFO_TYPE_ACCESS | INFO_TYPE_CONTENT));
    
    if (!status_from_stat) {
      return DataStatus(DataStatus::CheckError, status_from_stat.GetErrno());
    }
    
    SetSize(file.GetSize());
    SetModified(file.GetModified());
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::Stat(FileInfo& file, DataPointInfoType verb) {
    return do_stat(url, file, verb);
  }

  DataStatus DataPointGFAL::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    // Open the directory
    struct dirent *d;
    DIR *dir;    
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      dir = gfal_opendir(GFALUtils::GFALURL(url).c_str());
    }
    if (!dir) {
      logger.msg(VERBOSE, "gfal_opendir failed: %s", StrError(gfal_posix_code_error()));
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::ListError, error_no);
    }
    
    // Loop over the content of the directory
    while ((d = gfal_readdir (dir))) {
      // Create a new FileInfo object and add it to the list of files
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(d->d_name));
      // If information about times, type or access was also requested, do a stat
      if (verb & (INFO_TYPE_TIMES | INFO_TYPE_ACCESS | INFO_TYPE_TYPE)) {
        URL child_url = URL(url.plainstr() + '/' + d->d_name);
        logger.msg(DEBUG, "List will stat the URL %s", child_url.plainstr());
        do_stat(child_url, *f, verb);
      }
    }
    
    // Then close the dir
    if (gfal_closedir (dir) < 0) {
      logger.msg(WARNING, "gfal_closedir failed: %s", StrError(gfal_posix_code_error()));
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::ListError, error_no);
    }
    
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::Remove() {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC);
    FileInfo file;
    DataStatus status_from_stat = do_stat(url, file, (DataPointInfoType)(INFO_TYPE_TYPE));
    if (!status_from_stat)
      return DataStatus(DataStatus::DeleteError, status_from_stat.GetErrno());

    int res;
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);

      if (file.GetType() == FileInfo::file_type_dir) {
        res = gfal_rmdir(GFALUtils::GFALURL(url).c_str());
      } else {
        res = gfal_unlink(GFALUtils::GFALURL(url).c_str());
      }
    }
    if (res < 0) {
      if (file.GetType() == FileInfo::file_type_dir) {
        logger.msg(VERBOSE, "gfal_rmdir failed: %s", StrError(gfal_posix_code_error()));
      }
      else {
        logger.msg(VERBOSE, "gfal_unlink failed: %s", StrError(gfal_posix_code_error()));
      }
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::DeleteError, error_no);
    }
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::CreateDirectory(bool with_parents) {

    int res;
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      // gfal_mkdir is always recursive
      res = gfal_mkdir(GFALUtils::GFALURL(url).c_str(), 0700);
    }
    if (res < 0) {
      logger.msg(VERBOSE, "gfal_mkdir failed: %s", StrError(gfal_posix_code_error()));
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::CreateDirectoryError, error_no);
    }
    return DataStatus::Success;    
  }
  
  DataStatus DataPointGFAL::Rename(const URL& newurl) {

    int res;
    {
      GFALEnvLocker gfal_lock(usercfg, lfc_host);
      res = gfal_rename(GFALUtils::GFALURL(url).c_str(), GFALUtils::GFALURL(newurl).c_str());
    }
    if (res < 0) {
      logger.msg(VERBOSE, "gfal_rename failed: %s", StrError(gfal_posix_code_error()));
      int error_no = GFALUtils::HandleGFALError(logger);
      return DataStatus(DataStatus::RenameError, error_no);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGFAL::Transfer3rdParty(const URL& source, const URL& destination, DataPoint::TransferCallback callback) {

    if (source.Protocol() == "lfc") lfc_host = source.Host();
    GFALEnvLocker gfal_lock(usercfg, lfc_host);
    GFALTransfer3rdParty transfer(source, destination, usercfg, callback);
    return transfer.Transfer();
  }

  bool DataPointGFAL::RequiresCredentialsInFile() const {
    return true;
  }



} // namespace ArcDMCGFAL

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "gfal2", "HED:DMC", "Grid File Access Library 2", 0, &ArcDMCGFAL::DataPointGFAL::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
