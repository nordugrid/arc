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

namespace Arc {

  /// Class for locking environment while calling gfal functions.
  class GFALEnvLocker: public CertEnvLocker {
  public:
    static Logger logger;
    GFALEnvLocker(const UserConfig& usercfg): CertEnvLocker(usercfg) {
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
      EnvLockWrap(false);
    }
  };

  Logger GFALEnvLocker::logger(Logger::getRootLogger(), "GFALEnvLocker");

  static char const * const stdfds[] = {
    "stdin",
    "stdout",
    "stderr"
  };

  Logger DataPointGFAL::logger(Logger::getRootLogger(), "DataPoint.GFAL");

  DataPointGFAL::DataPointGFAL(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg), reading(false), writing(false) {
      LogLevel loglevel = logger.getThreshold();
      if (loglevel == DEBUG)
        gfal_set_verbose (GFAL_VERBOSE_VERBOSE | GFAL_VERBOSE_TRACE);
      if (loglevel == VERBOSE)
        gfal_set_verbose (GFAL_VERBOSE_VERBOSE);
  }

  DataPointGFAL::~DataPointGFAL() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointGFAL::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "rfio")
      return NULL;
    return new DataPointGFAL(*dmcarg, *dmcarg, dmcarg);
  }

  DataStatus DataPointGFAL::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    
    // Open the file
    {
      GFALEnvLocker gfal_lock(usercfg);
      if ((fd = gfal_open(url.plainstr().c_str(), O_RDONLY, 0)) < 0) {
        logger.msg(ERROR, "gfal_open failed: %s", StrError(errno));
        log_gfal_err();
        reading = false;
        return DataStatus::ReadStartError;
      }
    }
    
    // Remember the DataBuffer we got: the separate reading thread will use it
    buffer = &buf;
    // StopReading will wait for this condition,
    // which will be signalled by the separate reading thread
    transfer_condition.reset();
    // Create the separate reading thread
    if (!CreateThreadFunction(&DataPointGFAL::read_file_start, this)) {
      logger.msg(ERROR, "Failed to create reading thread");
      if (fd != -1) {
        if (gfal_close(fd) < 0) {
          logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
        }
      }
      reading = false;
      return DataStatus::ReadStartError;
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
    unsigned int bytes_read;
    for (;;) {
      // Ask the DataBuffer for a buffer to read into
      if (!buffer->for_read(handle, length, true)) {
        buffer->error_read(true);
        break;
      }

      // Read into the buffer
      bytes_read = gfal_read(fd, (*(buffer))[handle], length);
            
      // If there was an error
      if (bytes_read < 0) {
        logger.msg(ERROR, "gfal_read failed: %s", StrError(errno));
        log_gfal_err();
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
      if (gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
      }
      fd = -1;
    }
    // Signal that we finished reading (even if there was an error)
    transfer_condition.signal();
  }
  
  DataStatus DataPointGFAL::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;
    // If the reading is not finished yet trigger reading error
    if (!buffer->eof_read()) buffer->error_read(true);

    // Wait for the reading thread to finish
    logger.msg(DEBUG, "StopReading starts waiting for transfer_condition.");
    transfer_condition.wait();
    logger.msg(DEBUG, "StopReading finished waiting for transfer_condition.");

    // Close the file if not already done
    if (fd != -1) {
      if (gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
      }
      fd = -1;
    }
    // If there was an error (maybe we triggered it)
    if (buffer->error_read())
      return DataStatus::ReadError;
    // If there was no error (the reading already finished)
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::StartWriting(DataBuffer& buf, DataCallback *space_cb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    writing = true;

    {
      GFALEnvLocker gfal_lock(usercfg);

      // Create the parent directory
      URL parent_url = URL(url.plainstr());
      parent_url.ChangePath(Glib::path_get_dirname(url.Path()));
      if (gfal_mkdir(parent_url.plainstr().c_str(), 0700) < 0) {
        logger.msg(DEBUG, "Failed to create parent directory, continuing anyway: %s", StrError(errno));
      }

      // Open the file
      if ((fd = gfal_open(url.plainstr().c_str(), O_WRONLY | O_CREAT, 0600)) < 0) {
        logger.msg(ERROR, "gfal_open failed: %s", StrError(errno));
        log_gfal_err();
        writing = false;
        return DataStatus::WriteStartError;
      }
    }
    
    // Remember the DataBuffer we got, the separate writing thread will use it
    buffer = &buf;
    // StopWriting will wait for this condition,
    // which will be signalled by the separate writing thread
    transfer_condition.reset();
    // Create the separate writing thread
    if (!CreateThreadFunction(&DataPointGFAL::write_file_start, this)) {
      logger.msg(ERROR, "Failed to create writing thread");
      if (fd != -1) {
        if (gfal_close(fd) < 0) {
          logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
        }
      }
      writing = false;
      return DataStatus::WriteStartError;
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
    unsigned int bytes_written;
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
        gfal_lseek(fd, position, SEEK_SET);
        offset = position;
      }
      
      // we want to write the chunk we got from the buffer,
      // but we may not be able to write it in one shot
      chunk_offset = 0;
      while (chunk_offset < length) {
        bytes_written = gfal_write(fd, (*(buffer))[handle] + chunk_offset, length - chunk_offset);
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
        logger.msg(ERROR, "gfal_write failed: %s", StrError(errno));
        log_gfal_err();
        buffer->error_write(true);
        break;
      }
    }
    buffer->eof_write(true);
    // Close the file
    if (fd != -1) {
      if (gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
      }
      fd = -1;
    }
    // Signal that we finished writing (even if there was an error)
    transfer_condition.signal();
  }
    
  DataStatus DataPointGFAL::StopWriting() {
    if (!writing) return DataStatus::WriteStopError;
    writing = false;
    
    // If the writing is not finished, trigger writing error
    if (!buffer->eof_write()) buffer->error_write(true);

    // Wait until the writing thread finishes
    logger.msg(DEBUG, "StopWriting starts waiting for transfer_condition.");
    transfer_condition.wait();
    logger.msg(DEBUG, "StopWriting finished waiting for transfer_condition.");

    // Close the file if not done already
    if (fd != -1) {
      if (gfal_close(fd) < 0) {
        logger.msg(WARNING, "gfal_close failed: %s", StrError(errno));
      }
      fd = -1;
    }
    // If there was an error (maybe we triggered it)
    if (buffer->error_write())
      return DataStatus::WriteError;
    return DataStatus::Success;
  }  
  
  DataStatus DataPointGFAL::do_stat(URL stat_url, FileInfo& file) {
    struct stat st;

    {
      GFALEnvLocker gfal_lock(usercfg);

      if (gfal_stat(stat_url.plainstr().c_str(), &st) < 0) {
        logger.msg(ERROR, "gfal_stat failed: %s", StrError(errno));
        log_gfal_err();
        return DataStatus::StatError;
      }
    }

    if(S_ISREG(st.st_mode)) {
      file.SetType(FileInfo::file_type_file);
      file.SetMetaData("type", "file");
    } else if(S_ISDIR(st.st_mode)) {
      file.SetType(FileInfo::file_type_dir);
      file.SetMetaData("type", "dir");
    } else {
      file.SetType(FileInfo::file_type_unknown);
    }

    std::string path = stat_url.Path();
    std::string name = Glib::path_get_basename(path);
    file.SetMetaData("path", path);
    file.SetName(name);

    file.SetSize(st.st_size);
    file.SetMetaData("size", tostring(st.st_size));
    file.SetCreated(st.st_mtime);
    file.SetMetaData("mtime", (Time(st.st_mtime)).str());
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

    return DataStatus::Success;    
  }

  DataStatus DataPointGFAL::Check() {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    
    FileInfo file;
    DataStatus status_from_stat = do_stat(url, file);
    
    if (status_from_stat != DataStatus::Success) {
      return DataStatus::CheckError;
    }
    
    SetSize(file.GetSize());
    SetCreated(file.GetCreated());
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::Stat(FileInfo& file, DataPointInfoType verb) {
    return do_stat(url, file);
  }

  DataStatus DataPointGFAL::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    // Open the directory
    struct dirent *d;
    DIR *dir;    
    {
      GFALEnvLocker gfal_lock(usercfg);
      if ((dir = gfal_opendir(url.plainstr().c_str())) == NULL) {
        logger.msg(ERROR, "gfal_opendir failed: %s", StrError(errno));
        log_gfal_err();
        return DataStatus::ListError;
      }
    }
    
    // Loop over the content of the directory
    while ((d = gfal_readdir (dir))) {
      // Create a new FileInfo object and add it to the list of files
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(d->d_name));
      DataStatus status_from_stat = DataStatus::StatError;
      // If information about times and access was also requested, do a stat
      if (verb & (INFO_TYPE_TIMES | INFO_TYPE_ACCESS)) {
        URL child_url = URL(url.plainstr() + '/' + d->d_name);
        logger.msg(DEBUG, "List will stat the URL %s", child_url.plainstr());
        status_from_stat = do_stat(child_url, *f);
      }
      // If something was not OK with Stat (or we didn't call it), just get the type
      if (status_from_stat != DataStatus::Success) {
        if (d->d_type == DT_DIR) {
          f->SetType(FileInfo::file_type_dir);
        } else if (d->d_type == DT_REG) {
          f->SetType(FileInfo::file_type_file);
        }
      }
    }
    
    // Then close the dir
    if (gfal_closedir (dir) < 0) {
      logger.msg(WARNING, "gfal_closedir failed: %s", StrError(errno));
      return DataStatus::ListError;
    }
    
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::Remove() {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsReadingError;
    FileInfo file;
    DataStatus status_from_stat = do_stat(url, file);
    if (status_from_stat != DataStatus::Success)
      return DataStatus::DeleteError;
    {
      GFALEnvLocker gfal_lock(usercfg);

      if (file.GetType() == FileInfo::file_type_dir) {
        if (gfal_rmdir(url.plainstr().c_str()) < 0) {
          logger.msg(ERROR, "gfal_rmdir failed: %s", StrError(errno));
          log_gfal_err();
          return DataStatus::DeleteError;
        }
      } else {
        if (gfal_unlink(url.plainstr().c_str()) < 0) {
          logger.msg(ERROR, "gfal_unlink failed: %s", StrError(errno));
          log_gfal_err();
          return DataStatus::DeleteError;
        }
      }
    }
    return DataStatus::Success;
  }
  
  DataStatus DataPointGFAL::CreateDirectory(bool with_parents) {

    GFALEnvLocker gfal_lock(usercfg);
    if (gfal_mkdir(url.plainstr().c_str(), 0700) < 0) {
      logger.msg(ERROR, "gfal_mkdir failed: %s", StrError(errno));
      log_gfal_err();
      return DataStatus::CreateDirectoryError;
    }
    return DataStatus::Success;    
  }
  
  void DataPointGFAL::log_gfal_err() {
    char errbuf[2048];
    gfal_posix_strerror_r(errbuf, sizeof(errbuf));
    logger.msg(ERROR, errbuf);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "rfio", "HED:DMC", "RFIO plugin using the GFAL2 library", 0, &Arc::DataPointGFAL::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
