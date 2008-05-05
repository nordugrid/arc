#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/URL.h>

#include <arc/data/MkDirRecursive.h>
#include <arc/data/CheckFile.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataCallback.h>
#define DIR_SEPARATOR '/'
#ifdef WIN32
#undef DIR_SEPARATOR
#define DIR_SEPARATOR '\\'
#include <arc/win32.h>
#endif

#include "DataPointFile.h"

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cerrno>

namespace Arc {

  Logger DataPointFile::logger(DataPoint::logger, "File");

  DataPointFile::DataPointFile(const URL& url) : DataPointDirect(url),
                                                 reading(false),
                                                 writing(false),
                                                 is_channel(false) {
    if (url.Protocol() == "file") {
      cache = false;
      is_channel = false;
    }
    else if (url.Path() == "-") {
      linkable = false;
      is_channel = true;
    }
  }

  DataPointFile::~DataPointFile() {
    StopReading();
    StopWriting();
  }

  void DataPointFile::read_file() {
    bool limit_length = false;
    unsigned long long int range_length = 0;
    unsigned long long int offset = 0;
    if (range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
      lseek(fd, range_start, SEEK_SET);
      offset = range_start;
    }
    else
      lseek(fd, 0, SEEK_SET);
    for(;;) {
      if (limit_length)
        if (range_length == 0)
          break;
      /* read from fd here and push to buffer */
      /* 1. claim buffer */
      int h;
      unsigned int l;
      if (!buffer->for_read(h, l, true)) {
        /* failed to get buffer - must be error or request to exit */
        buffer->error_read(true);
        break;
      }
      if (buffer->error()) {
        buffer->is_read(h, 0, 0);
        break;
      }
      /* 2. read */
      if (limit_length)
        if (l > range_length)
          l = range_length;
      unsigned long long int p = lseek(fd, 0, SEEK_CUR);
      if (p == (unsigned long long int)(-1))
        p = offset;
      int ll = read(fd, (*(buffer))[h], l);
      if (ll == -1) {/* error */
        buffer->is_read(h, 0, 0);
        buffer->error_read(true);
        break;
      }
      if (ll == 0) {/* eof */
        buffer->is_read(h, 0, 0);
        break;
      }
      /* 3. announce */
      buffer->is_read(h, ll, p);
      if (limit_length)
        if (ll > range_length)
          range_length = 0;
        else
          range_length -= ll;
      offset += ll; // for non-seakable files
    }
    close(fd);
    buffer->eof_read(true);
    transfer_cond.signal();
  }

  void DataPointFile::write_file() {
    for(;;) {
      /* take from buffer and write to fd */
      /* 1. claim buffer */
      int h;
      unsigned int l;
      unsigned long long int p;
      if (!buffer->for_write(h, l, p, true)) {
        /* failed to get buffer - must be error or request to exit */
        if (!buffer->eof_read())
          buffer->error_write(true);
        buffer->eof_write(true);
        break;
      }
      if (buffer->error()) {
        buffer->is_written(h);
        buffer->eof_write(true);
        break;
      }
      /* 2. write */
      lseek(fd, p, SEEK_SET);
      int l_ = 0;
      int ll = 0;
      while (l_ < l) {
        ll = write(fd, (*(buffer))[h] + l_, l - l_);
        if (ll == -1) {/* error */
          buffer->is_written(h);
          buffer->error_write(true);
          buffer->eof_write(true);
          break;
        }
        l_ += ll;
      }
      if (ll == -1)
        break;
      /* 3. announce */
      buffer->is_written(h);
    }
    close(fd);
    transfer_cond.signal();
  }

  DataStatus DataPointFile::Check() {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    Arc::User user;
    int res = user.check_file_access(url.Path(), O_RDONLY);
    if (res != 0) {
      logger.msg(INFO, "File is not accessible: %s", url.Path());
      return DataStatus::CheckError;
    }
    struct stat st;
    if (stat(url.Path().c_str(), &st) != 0) {
      logger.msg(INFO, "Can't stat file: %s", url.Path());
      return DataStatus::CheckError;
    }
    SetSize(st.st_size);
    SetCreated(st.st_mtime);
    return DataStatus::Success;
  }

  DataStatus DataPointFile::Remove() {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsReadingError;
    if (unlink(url.Path().c_str()) == -1)
      if (errno != ENOENT)
        return DataStatus::DeleteError;
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StartReading(DataBufferPar& buf) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    reading = true;
    /* try to open */
    int flags = O_RDONLY;
#ifdef WIN32
    flags |= O_BINARY;
#endif
    
    if (url.Path() == "-")
      fd = dup(STDIN_FILENO);
    else {
      Arc::User user;
      if (user.check_file_access(url.Path(), flags) != 0) {
        reading = false;
        return DataStatus::ReadStartError;
      }
      fd = open(url.Path().c_str(), flags);
    }
    if (fd == -1) {
      reading = false;
      return DataStatus::ReadStartError;
    }
    /* provide some metadata */
    struct stat st;
    if (fstat(fd, &st) == 0) {
      SetSize(st.st_size);
      SetCreated(st.st_mtime);
    }
    buffer = &buf;
    transfer_cond.reset();
    /* create thread to maintain reading */
    Glib::Thread* thr = NULL;
    try {
      thr = Glib::Thread::create(sigc::mem_fun(*this, &DataPointFile::read_file), false);
    }
    catch (std::exception& e) {}
    if (!thr) {
      close(fd);
      fd = -1;
      reading = false;
      return DataStatus::ReadStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StopReading() {
    if (!reading)
      return DataStatus::ReadStopError;
    reading = false;
    if (!buffer->eof_read()) {
      buffer->error_read(true);      /* trigger transfer error */
      close(fd);
      fd = -1;
    }
    // buffer->wait_eof_read();
    transfer_cond.wait();         /* wait till reading thread exited */
    if(buffer->error_read()) return DataStatus::ReadError;
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StartWriting(DataBufferPar& buf,
                                         DataCallback *space_cb) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    writing = true;
    /* try to open */
    buffer = &buf;
    if (url.Path() == "-") {
      fd = dup(STDOUT_FILENO);
      if (fd == -1) {
        logger.msg(ERROR, "Failed to use channel stdout");
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus::WriteStartError;
      }
    }
    else {
      Arc::User user;
      /* do not check permissions to create anything here -
         suppose it path was checked at higher level */
      /* make directories */
      if (url.Path().empty()) {
        logger.msg(ERROR, "Invalid url: %s", url.str());
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus::WriteStartError;
      }
      std::string dirpath = url.Path();
      int n = dirpath.rfind(DIR_SEPARATOR);
      if (n == 0)
        dirpath = "/";
      else
        dirpath.erase(n, dirpath.length() - n + 1);
      if (mkdir_recursive("", dirpath, S_IRWXU, user) != 0) {
        if (errno != EEXIST) {
          logger.msg(ERROR, "Failed to create/find directory %s", dirpath);
          buffer->error_write(true);
          buffer->eof_write(true);
          writing = false;
          return DataStatus::WriteStartError;
        }
      }

      /* try to create file, if failed - try to open it */
      int flags = O_WRONLY;
#ifdef WIN32
      flags |= O_BINARY;
#endif
      fd = open(url.Path().c_str(), flags | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
      if (fd == -1)
        fd = open(url.Path().c_str(), flags | O_TRUNC, S_IRUSR | S_IWUSR);
      else {/* this file was created by us. Hence we can set it's owner */
        (fchown(fd, user.get_uid(), user.get_gid()) != 0);
      }
      if (fd == -1) {
        logger.msg(ERROR, "Failed to create/open file %s (%d)", url.Path(), errno);
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus::WriteStartError;
      }

      /* preallocate space */
      buffer->speed.hold(true);
      if (CheckSize()) {
        unsigned long long int fsize = GetSize();
        logger.msg(INFO, "setting file %s to size %llu",
                   url.Path(), fsize);
        /* because filesytem can skip empty blocks do real write */
        unsigned long long int old_size = lseek(fd, 0, SEEK_END);
        if (old_size < fsize) {
          char buf[65536];
          memset(buf, 0xFF, sizeof(buf));
          unsigned int l = 1;
          while (l > 0) {
            old_size = lseek(fd, 0, SEEK_END);
            l = sizeof(buf);
            if (l > (fsize - old_size))
              l = fsize - old_size;
            if (write(fd, buf, l) == -1) {
              /* out of space */
              if (space_cb != NULL)
                if (space_cb->cb((unsigned long long int)l))
                  continue;
              lseek(fd, 0, SEEK_SET);
              (ftruncate(fd, 0) != 0);
              close(fd);
              fd = -1;
              logger.msg(INFO, "Failed to preallocate space");
              buffer->speed.reset();
              buffer->speed.hold(false);
              buffer->error_write(true);
              buffer->eof_write(true);
              writing = false;
              return DataStatus::WriteStartError;
            }
          }
        }
      }
    }
    buffer->speed.reset();
    buffer->speed.hold(false);
    transfer_cond.reset();
    /* create thread to maintain writing */
    Glib::Thread* thr = NULL;
    try {
      thr = Glib::Thread::create(sigc::mem_fun(*this, &DataPointFile::write_file), false);
    }
    catch (std::exception& e) {}
    if (!thr) {
      close(fd);
      fd = -1;
      buffer->error_write(true);
      buffer->eof_write(true);
      writing = false;
      return DataStatus::WriteStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StopWriting() {
    if (!writing)
      return DataStatus::WriteStopError;
    writing = false;
    if (!buffer->eof_write()) {
      buffer->error_write(true);      /* trigger transfer error */
      close(fd);
      fd = -1;
    }
    // buffer->wait_eof_write();
    transfer_cond.wait();         /* wait till writing thread exited */
    if(buffer->error_write()) return DataStatus::WriteError;
    return DataStatus::Success;
  }

  DataStatus DataPointFile::ListFiles(std::list<FileInfo>& files,
                                      bool resolve) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    std::string dirname = url.Path();
    if (dirname[dirname.length() - 1] == '/')
      dirname.resize(dirname.length() - 1);
    
    try {
        Glib::Dir dir(dirname);
        std::string file_name;
        while ((file_name = dir.read_name()) != "") {
            std::list<FileInfo>::iterator f =
                files.insert(files.end(), FileInfo(file_name.c_str()));
            if (resolve) {
                std::string fname = dirname + "/" + file_name;
                struct stat st;
                if (stat(fname.c_str(), &st) == 0) {
                    f->SetSize(st.st_size);
                    f->SetCreated(st.st_mtime);
                    if (S_ISDIR(st.st_mode))
                        f->SetType(FileInfo::file_type_dir);
                    else if (S_ISREG(st.st_mode))
                        f->SetType(FileInfo::file_type_file);
                }
            }
        }
    } catch (Glib::FileError &e) {
      // maybe a file
      struct stat st;
      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(dirname));
      if (stat(dirname.c_str(), &st) == 0) {
        f->SetSize(st.st_size);
        f->SetCreated(st.st_mtime);
        if (S_ISDIR(st.st_mode))
          f->SetType(FileInfo::file_type_dir);
        else if (S_ISREG(st.st_mode))
          f->SetType(FileInfo::file_type_file);
        return DataStatus::Success;
      }
      files.erase(f);
      logger.msg(INFO, "Failed to read object: %s", dirname);
      return DataStatus::ListError;
    }
    return DataStatus::Success;
  }

  bool DataPointFile::WriteOutOfOrder() {
    if (!url)
      return false;
    if (url.Protocol() == "file")
      return true;
    return false;
  }

} // namespace Arc
