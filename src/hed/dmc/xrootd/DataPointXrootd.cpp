#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>

#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>

#include "DataPointXrootd.h"

namespace ArcDMCXrootd {

  using namespace Arc;

  Logger DataPointXrootd::logger(Logger::getRootLogger(), "DataPoint.Xrootd");
  XrdPosixXrootd DataPointXrootd::xrdposix;

  DataPointXrootd::DataPointXrootd(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      fd(-1),
      reading(false),
      writing(false){
    // set xrootd log level
    set_log_level();
  }

  DataPointXrootd::~DataPointXrootd() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointXrootd::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "root")
      return NULL;
    return new DataPointXrootd(*dmcarg, *dmcarg, dmcarg);
  }

  void DataPointXrootd::read_file_start(void* arg) {
    ((DataPointXrootd*)arg)->read_file();
  }

  void DataPointXrootd::read_file() {

    // TODO range reads
    bool do_cksum = true;
    unsigned long long int offset = 0;
    bool eof = false;

    for (;;) {
      // 1. claim buffer
      int h;
      unsigned int l;
      if (!buffer->for_read(h, l, true)) {
        // failed to get buffer - must be error or request to exit
        buffer->error_read(true);
        break;
      }
      if (buffer->error()) {
        buffer->is_read(h, 0, 0);
        break;
      }

      if (eof) {
        buffer->is_read(h, 0, 0);
        if(do_cksum) {
          for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                    cksum != checksums.end(); ++cksum) {
            if(*cksum) (*cksum)->end();
          }
        }
        break;
      }
      // 2. read, making sure not to read past eof
      if (size - offset < l) {
        l = size - offset;
        eof = true;
        if (l == 0) { // don't try to read zero bytes!
          buffer->is_read(h, 0, 0);
          continue;
        }
      }
      logger.msg(DEBUG, "Reading %u bytes from byte %llu", l, offset);
      int res = XrdPosixXrootd::Read(fd, (*(buffer))[h], l);
      logger.msg(DEBUG, "Read %i bytes", res);
      if (res <= 0) { // error
        buffer->is_read(h, 0, 0);
        buffer->error_read(true);
        break;
      }

      if(do_cksum) {
        for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                  cksum != checksums.end(); ++cksum) {
          if(*cksum) (*cksum)->add((*(buffer))[h], res);
        }
      }
      // 3. announce
      buffer->is_read(h, res, offset);
      offset += res;
    }
    XrdPosixXrootd::Close(fd);
    buffer->eof_read(true);
    transfer_cond.signal();
  }

  DataStatus DataPointXrootd::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;

    {
      CertEnvLocker env(usercfg);
      fd = XrdPosixXrootd::Open(url.str().c_str(), O_RDONLY);
      if (fd == -1) {
        logger.msg(VERBOSE, "Could not open file %s for reading: %s", url.str(), StrError(errno));
        reading = false;
        return DataStatus(DataStatus::ReadStartError, errno);
      }
    }

    // It is an error to read past EOF, so we need the file size if not known
    if (!CheckSize()) {
      FileInfo f;
      DataStatus res = Stat(f, INFO_TYPE_CONTENT);
      if (!res) {
        reading = false;
        return DataStatus(DataStatus::ReadStartError, res.GetErrno(), res.GetDesc());
      }
      if (!CheckSize()) {
        logger.msg(VERBOSE, "Unable to find file size of %s", url.str());
        reading = false;
        return DataStatus(DataStatus::ReadStartError, std::string("Unable to obtain file size"));
      }
    }

    buffer = &buf;
    transfer_cond.reset();
    // create thread to maintain reading
    if(!CreateThreadFunction(&DataPointXrootd::read_file_start, this)) {
      XrdPosixXrootd::Close(fd);
      reading = false;
      buffer = NULL;
      return DataStatus::ReadStartError;
    }

    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;
    if (!buffer) return DataStatus(DataStatus::ReadStopError, EARCLOGIC, "Not reading");
    if (!buffer->eof_read()) {
      buffer->error_read(true);      /* trigger transfer error */
      if (fd != -1) XrdPosixXrootd::Close(fd);
      fd = -1;
    }
    transfer_cond.wait();         /* wait till reading thread exited */
    if (buffer->error_read()) {
      buffer = NULL;
      return DataStatus::ReadError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }


  DataStatus DataPointXrootd::StartWriting(DataBuffer& buffer,
                                      DataCallback *space_cb) {
    logger.msg(ERROR, "Writing to xrootd is not (yet) supported");
    return DataStatus(DataStatus::WriteError, EOPNOTSUPP);
  }

  DataStatus DataPointXrootd::StopWriting() {
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Check(bool check_meta) {
    // check if file can be opened for reading
    CertEnvLocker env(usercfg);
    if (XrdPosixXrootd::Access(url.str().c_str(), R_OK) != 0) {
      logger.msg(VERBOSE, "Read access not allowed for %s: %s", url.str(), StrError(errno));
      return DataStatus(DataStatus::CheckError, errno);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::do_stat(const URL& url, FileInfo& file, DataPointInfoType verb) {

    struct stat st;
    {
      CertEnvLocker env(usercfg);
      if (XrdPosixXrootd::Stat(url.str().c_str(), &st)) {
        logger.msg(VERBOSE, "Could not stat file %s: %s", url.str(), StrError(errno));
        return DataStatus(DataStatus::StatError, errno);
      }
    }
    file.SetName(url.Path());
    file.SetSize(st.st_size);
    file.SetModified(st.st_mtime);

    if(S_ISREG(st.st_mode)) {
      file.SetType(FileInfo::file_type_file);
    } else if(S_ISDIR(st.st_mode)) {
      file.SetType(FileInfo::file_type_dir);
    } else {
      file.SetType(FileInfo::file_type_unknown);
    }

    SetSize(file.GetSize());
    SetModified(file.GetModified());

    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Stat(FileInfo& file, DataPointInfoType verb) {
    return do_stat(url, file, verb);
  }

  DataStatus DataPointXrootd::List(std::list<FileInfo>& files, DataPointInfoType verb) {

    DIR* dir = NULL;
    {
      CertEnvLocker env(usercfg);
      dir = XrdPosixXrootd::Opendir(url.str().c_str());
    }
    if (!dir) {
      logger.msg(VERBOSE, "Failed to open directory %s: %s", url.str(), StrError(errno));
      return DataStatus(DataStatus::ListError, errno);
    }

    struct dirent* entry;
    while ((entry = XrdPosixXrootd::Readdir(dir))) {
      FileInfo f;
      if (verb > INFO_TYPE_NAME) {
        std::string path = url.str() + '/' + entry->d_name;
        do_stat(path, f, verb);
      }
      f.SetName(entry->d_name);
      files.push_back(f);
    }
    if (errno != 0) logger.msg(VERBOSE, "Error while reading dir %s: %s", url.str(), StrError(errno));
    XrdPosixXrootd::Closedir(dir);

    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Remove() {
    logger.msg(ERROR, "Cannot (yet) remove files through xrootd");
    return DataStatus(DataStatus::DeleteError, EOPNOTSUPP);
  }

  DataStatus DataPointXrootd::CreateDirectory(bool with_parents) {
    logger.msg(ERROR, "Cannot (yet) create directories through xrootd");
    return DataStatus(DataStatus::CreateDirectoryError, EOPNOTSUPP);
  }

  DataStatus DataPointXrootd::Rename(const URL& newurl) {
    logger.msg(ERROR, "Cannot (yet) rename files through xrootd");
    return DataStatus(DataStatus::RenameError, EOPNOTSUPP);
  }

  void DataPointXrootd::set_log_level() {
    // TODO xrootd lib logs to stderr - need to redirect to log file for DTR
    // Level 1 enables some messages which go to stdout - which messes up
    // communication in DTR so better to use no debugging
    if (logger.getThreshold() == DEBUG) XrdPosixXrootd::setDebug(1);
    else XrdPosixXrootd::setDebug(0);
  }

} // namespace ArcDMCXrootd


Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "root", "HED:DMC", "XRootd", 0, &ArcDMCXrootd::DataPointXrootd::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
