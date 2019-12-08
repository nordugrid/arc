#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <XrdCl/XrdClPropertyList.hh>
#include <XrdCl/XrdClDefaultEnv.hh>
#include <XrdCl/XrdClLog.hh>

#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>

#include "DataPointXrootd.h"

namespace ArcDMCXrootd {

  using namespace Arc;

  Logger DataPointXrootd::logger(Logger::getRootLogger(), "DataPoint.Xrootd");
  XrdPosixXrootd DataPointXrootd::xrdposix;

  XrootdProgressHandler::XrootdProgressHandler(DataPoint::TransferCallback callback)
   : cb(callback), cancel(false) {}

  void XrootdProgressHandler::JobProgress(uint16_t jobNum,
                                          uint64_t bytesProcessed,
                                          uint64_t bytesTotal) {
    cb(bytesProcessed);
  }

  bool XrootdProgressHandler::ShouldCancel(uint64_t jobNum) {
    return cancel;
  }


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
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Xrootd in non-persistent mode - Xrootd code is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);

    return new DataPointXrootd(*dmcarg, *dmcarg, dmcarg);
  }

  DataStatus DataPointXrootd::copy_file(std::string source, std::string dest, TransferCallback callback) {
    XrdCl::PropertyList props;
    XrdCl::PropertyList results;

    // Check for special source/dest to handle
    if (source.find("file:/") == 0) {
      // xrootd doesn't like the arc file:/ urls so remove the protocol
      source = source.substr(5);
    }
    if (source == "stdio:/stdin") {
      source = "-";
    }

    if (dest.find("file:/") == 0) {
      // xrootd doesn't like the arc file:/ urls so remove the protocol
      dest = dest.substr(5);
    }
    if (dest == "stdio:/stdout") {
      dest = "-";
    }
    props.Set("source", source);
    props.Set("target", dest);
    // If checksum is known then ask xrootd to check it
    if (CheckCheckSum()) {
      std::list<std::string> csum;
      tokenize(GetCheckSum(), csum, ":");
      if (csum.size() == 2) {
        props.Set("checkSumMode", "end2end");
        props.Set("checkSumType", csum.front());
        props.Set("checkSumPreset", csum.back());
      } else {
        logger.msg(WARNING, "Could not handle checksum %s: skip checksum check", GetCheckSum());
      }
    }
    XrdCl::CopyProcess copy;
    XrdCl::XRootDStatus st = copy.AddJob(props, &results);
    if (!st.IsOK()) {
      logger.msg(ERROR, "Failed to create xrootd copy job: %s", st.ToStr());
      return DataStatus(DataStatus::TransferError, EIO, st.ToStr());
    }

    XrdCl::PropertyList processConfig;
    processConfig.Set("jobType", "configuration" );
    processConfig.Set("parallel", 1 );
    copy.AddJob(processConfig, 0 );
    copy.Prepare();

    XrdCl::CopyProgressHandler * cph = NULL;
    if (callback) {
      cph = new XrootdProgressHandler(callback);
    }
    st = copy.Run(cph);
    if (cph) delete cph;
    if (!st.IsOK()) {
      logger.msg(ERROR, "Failed to copy %s: %s", source, st.ToStr());
      // xrootd defines its own error codes so return generic I/O error
      return DataStatus(DataStatus::TransferError, EIO, st.ToStr());
    }
    return DataStatus::Success;
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
      fd = XrdPosixXrootd::Open(url.plainstr().c_str(), O_RDONLY);
      if (fd == -1) {
        logger.msg(VERBOSE, "Could not open file %s for reading: %s", url.plainstr(), StrError(errno));
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
        logger.msg(VERBOSE, "Unable to find file size of %s", url.plainstr());
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

  void DataPointXrootd::write_file_start(void *object) {
    ((DataPointXrootd*)object)->write_file();
  }

  void DataPointXrootd::write_file() {
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
        logger.msg(DEBUG, "DataPointXrootd::write_file got position %d and offset %d, has to seek", position, offset);
        XrdPosixXrootd::Lseek(fd, position, SEEK_SET);
        offset = position;
      }

      // we want to write the chunk we got from the buffer,
      // but we may not be able to write it in one shot
      chunk_offset = 0;
      while (chunk_offset < length) {
        bytes_written = XrdPosixXrootd::Write(fd, (*(buffer))[handle] + chunk_offset, length - chunk_offset);
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
        logger.msg(VERBOSE, "xrootd write failed: %s", StrError(errno));
        buffer->error_write(true);
        break;
      }
    }
    buffer->eof_write(true);
    // Close the file
    if (fd != -1) {
      if (XrdPosixXrootd::Close(fd) < 0) {
        logger.msg(WARNING, "xrootd close failed: %s", StrError(errno));
      }
      fd = -1;
    }
    transfer_cond.signal();
  }


  DataStatus DataPointXrootd::StartWriting(DataBuffer& buf,
                                           DataCallback *space_cb) {

    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    writing = true;

    {
      CertEnvLocker env(usercfg);
      // Open the file
      fd = XrdPosixXrootd::Open(url.plainstr().c_str(), O_WRONLY | O_CREAT, 0600);
    }
    if (fd < 0) {
      // If no entry try to create parent directories
      if (errno == ENOENT) {
        logger.msg(VERBOSE, "Failed to open %s, trying to create parent directories", url.plainstr());
        std::string original_path(url.Path());
        url.ChangePath(Glib::path_get_dirname(url.Path()));
        DataStatus r = CreateDirectory(true);
        url.ChangePath(original_path);
        if (!r) return r;

        { // Try to open again
          CertEnvLocker env(usercfg);
          fd = XrdPosixXrootd::Open(url.plainstr().c_str(), O_WRONLY | O_CREAT, 0600);
        }
      }
      if (fd < 0) {
        logger.msg(VERBOSE, "xrootd open failed: %s", StrError(errno));
        writing = false;
        return DataStatus(DataStatus::WriteStartError, errno);
      }
    }

    // Remember the DataBuffer we got, the separate writing thread will use it
    buffer = &buf;
    transfer_cond.reset();
    // StopWriting will wait for this condition,
    // which will be signalled by the separate writing thread
    // Create the separate writing thread
    if (!CreateThreadFunction(&DataPointXrootd::write_file_start, this)) {
      if (fd != -1 && XrdPosixXrootd::Close(fd) < 0) {
        logger.msg(WARNING, "close failed: %s", StrError(errno));
      }
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create writing thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::StopWriting() {
    if (!writing) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");
    writing = false;
    if (!buffer) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");

    // If the writing is not finished, trigger writing error
    if (!buffer->eof_write()) buffer->error_write(true);

    // Wait until the writing thread finishes
    logger.msg(DEBUG, "StopWriting starts waiting for transfer_condition.");
    transfer_cond.wait();
    logger.msg(DEBUG, "StopWriting finished waiting for transfer_condition.");

    // Close the file if not done already
    if (fd != -1) {
      if (XrdPosixXrootd::Close(fd) < 0) {
        logger.msg(WARNING, "xrootd close failed: %s", StrError(errno));
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

  DataStatus DataPointXrootd::Check(bool check_meta) {

    {
      CertEnvLocker env(usercfg);
      if (XrdPosixXrootd::Access(url.plainstr().c_str(), R_OK) != 0) {
        logger.msg(VERBOSE, "Read access not allowed for %s: %s", url.plainstr(), StrError(errno));
        return DataStatus(DataStatus::CheckError, errno);
      }
    }
    if (check_meta) {
      FileInfo f;
      return do_stat(url, f, INFO_TYPE_CONTENT);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::do_stat(const URL& u, FileInfo& file, DataPointInfoType verb) {

    struct stat st;
    {
      CertEnvLocker env(usercfg);
      // When used against dcache stat returns 0 even if file does not exist
      // so check inode number
      if (XrdPosixXrootd::Stat(u.plainstr().c_str(), &st) != 0 || st.st_ino == (unsigned long long int)(-1)) {
        logger.msg(VERBOSE, "Could not stat file %s: %s", u.plainstr(), StrError(errno));
        return DataStatus(DataStatus::StatError, errno);
      }
      if (verb & INFO_TYPE_CONTENT) {
        if (u.HTTPOption("xrdcl.unzip") != "") {
          logger.msg(WARNING, "Not getting checksum of zip constituent");
        } else {
          char buf[256];
          if (XrdPosixXrootd::Getxattr(u.plainstr().c_str(), "xroot.cksum", buf, sizeof(buf)) == -1) {
            logger.msg(WARNING, "Could not get checksum of %s: %s", u.plainstr(), StrError(errno));
          } else {
            std::string csum(buf);
            if (csum.find(' ') != std::string::npos) csum.replace(csum.find(' '), 1, ":");
            logger.msg(VERBOSE, "Checksum %s", csum);
            file.SetCheckSum(csum);
            SetCheckSum(csum);
          }
        }
      }
    }

    file.SetName(u.Path());
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
      dir = XrdPosixXrootd::Opendir(url.plainstr().c_str());
    }
    if (!dir) {
      logger.msg(VERBOSE, "Failed to open directory %s: %s", url.plainstr(), StrError(errno));
      return DataStatus(DataStatus::ListError, errno);
    }

    struct dirent* entry;
    errno = 0; // reset because it is used as error indicator
    while ((entry = XrdPosixXrootd::Readdir(dir))) {
      FileInfo f;
      if (verb > INFO_TYPE_NAME) {
        std::string path = url.plainstr() + '/' + entry->d_name;
        do_stat(path, f, verb);
      }
      f.SetName(entry->d_name);
      files.push_back(f);
      errno = 0;
    }
    if (errno != 0) {
      int errNo = errno;
      logger.msg(VERBOSE, "Error while reading dir %s: %s", url.plainstr(), StrError(errNo));
      return DataStatus(DataStatus::ListError, errNo);
    }
    XrdPosixXrootd::Closedir(dir);

    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Remove() {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);

    struct stat st;
    CertEnvLocker env(usercfg);
    if (XrdPosixXrootd::Stat(url.plainstr().c_str(), &st) != 0) {
      if (errno == ENOENT) return DataStatus::Success;
      logger.msg(VERBOSE, "File is not accessible %s: %s", url.plainstr(), StrError(errno));
      return DataStatus(DataStatus::DeleteError, errno, "Failed to stat file "+url.plainstr());
    }
    // path is a directory
    if (S_ISDIR(st.st_mode)) {
      if (XrdPosixXrootd::Rmdir(url.plainstr().c_str()) != 0) {
        logger.msg(VERBOSE, "Can't delete directory %s: %s", url.plainstr(), StrError(errno));
        return DataStatus(DataStatus::DeleteError, errno, "Failed to delete directory "+url.plainstr());
      }
      return DataStatus::Success;
    }
    // path is a file
    if (XrdPosixXrootd::Unlink(url.plainstr().c_str()) != 0) {
      logger.msg(VERBOSE, "Can't delete file %s: %s", url.plainstr(), StrError(errno));
      return DataStatus(DataStatus::DeleteError, errno, "Failed to delete file "+url.plainstr());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::CreateDirectory(bool with_parents) {

    std::string::size_type slashpos = url.Path().find("/", 1); // don't create root dir
    int r;
    URL dir(url);

    if (!with_parents) {
      dir.ChangePath(url.Path().substr(0, url.Path().rfind("/")));
      if (dir.Path().empty() || dir == url.Path()) return DataStatus::Success;
      logger.msg(VERBOSE, "Creating directory %s", dir.plainstr());

      CertEnvLocker env(usercfg);
      r = XrdPosixXrootd::Mkdir(dir.plainstr().c_str(), 0775);
      if (r == 0 || errno == EEXIST) return DataStatus::Success;

      logger.msg(VERBOSE, "Error creating required dirs: %s", StrError(errno));
      return DataStatus(DataStatus::CreateDirectoryError, errno, StrError(errno));
    }
    while (slashpos != std::string::npos) {
      dir.ChangePath(url.Path().substr(0, slashpos));
      // stat dir to see if it exists
      struct stat st;
      CertEnvLocker env(usercfg);
      r = XrdPosixXrootd::Stat(dir.plainstr().c_str(), &st);
      if (r == 0) {
        slashpos = url.Path().find("/", slashpos + 1);
        continue;
      }

      logger.msg(VERBOSE, "Creating directory %s", dir.plainstr());
      r = XrdPosixXrootd::Mkdir(dir.plainstr().c_str(), 0775);
      if (r != 0) {
        if (errno != EEXIST) {
          logger.msg(VERBOSE, "Error creating required dirs: %s", StrError(errno));
          return DataStatus(DataStatus::CreateDirectoryError, errno, StrError(errno));
        }
      }
      slashpos = url.Path().find("/", slashpos + 1);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Rename(const URL& newurl) {
    logger.msg(VERBOSE, "Renaming %s to %s", url.plainstr(), newurl.plainstr());

    URL tmpurl(newurl);
    // xrootd requires 2 slashes at the start of the URL path
    if (tmpurl.Path().find("//") != 0) {
      tmpurl.ChangePath(std::string("/"+tmpurl.Path()));
    }

    if (XrdPosixXrootd::Rename(url.plainstr().c_str(), tmpurl.plainstr().c_str()) != 0) {
      logger.msg(VERBOSE, "Can't rename file %s: %s", url.plainstr(), StrError(errno));
      return DataStatus(DataStatus::RenameError, errno, "Failed to rename file "+url.plainstr());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Transfer(const URL& otherendpoint, bool source, TransferCallback callback) {
    if (source) {
      return copy_file(url.plainstr(), otherendpoint.plainstr(), callback);
    } else {
      return copy_file(otherendpoint.plainstr(), url.plainstr(), callback);
    }
  }

  bool DataPointXrootd::SupportsTransfer() const {
    return true;
  }

  bool DataPointXrootd::RequiresCredentialsInFile() const {
    return true;
  }

  void DataPointXrootd::set_log_level() {
    // TODO xrootd lib logs to stderr - need to redirect to log file for DTR
    // Level 1 enables some messages which go to stdout - which messes up
    // communication in DTR so better to use no debugging
    XrdCl::Log *log = XrdCl::DefaultEnv::GetLog();
    if (logger.getThreshold() == DEBUG) {
      XrdPosixXrootd::setDebug(1);
      log->SetLevel(XrdCl::Log::DumpMsg);
    }
    else {
      XrdPosixXrootd::setDebug(0);
      log->SetLevel(XrdCl::Log::ErrorMsg);
    }
  }

} // namespace ArcDMCXrootd


extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "root", "HED:DMC", "XRootd", 0, &ArcDMCXrootd::DataPointXrootd::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
