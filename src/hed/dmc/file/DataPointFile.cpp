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

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
#include <arc/data/CheckSum.h>
#include <arc/FileUtils.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "DataPointFile.h"

namespace Arc {

  Logger DataPointFile::logger(DataPoint::logger, "File");

  DataPointFile::DataPointFile(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      reading(false),
      writing(false),
      is_channel(false) {
    if (url.Protocol() == "file") {
      cache = false;
      is_channel = false;
      local = true;
    }
    else if (url.Path() == "-") { // won't work
      linkable = false;
      is_channel = true;
    }
  }

  DataPointFile::~DataPointFile() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointFile::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "file")
      return NULL;
    return new DataPointFile(*dmcarg, *dmcarg);
  }

  void DataPointFile::read_file_start(void* arg) {
    ((DataPointFile*)arg)->read_file();
  }

  void DataPointFile::read_file() {
    bool limit_length = false;
    unsigned long long int range_length = 0;
    unsigned long long int offset = 0;
    bool do_cksum = true;
    if (range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
      lseek(fd, range_start, SEEK_SET);
      offset = range_start;
      if(offset > 0) {
        // Note: checksum calculation not possible
        do_cksum = false;
      }
    }
    else
      lseek(fd, 0, SEEK_SET);
    for (;;) {
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
      if (ll == -1) { /* error */
        buffer->is_read(h, 0, 0);
        buffer->error_read(true);
        break;
      }
      if (ll == 0) { /* eof */
        buffer->is_read(h, 0, 0);
        if(do_cksum) {
          for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                    cksum != checksums.end(); ++cksum) {
            if(*cksum) (*cksum)->end();
          }
        }
        break;
      }
      if(do_cksum) {
        for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                  cksum != checksums.end(); ++cksum) {
          if(*cksum) (*cksum)->add((*(buffer))[h], ll);
        }
      }
      /* 3. announce */
      buffer->is_read(h, ll, p);
      if (limit_length) {
        if (ll > range_length)
          range_length = 0;
        else
          range_length -= ll;
      }
      offset += ll; // for non-seakable files
    }
    close(fd);
    buffer->eof_read(true);
    transfer_cond.signal();
  }

  void DataPointFile::write_file_start(void* arg) {
    ((DataPointFile*)arg)->write_file();
  }

  class write_file_chunks {
   private:
    typedef struct {
      unsigned long long int start;
      unsigned long long int end;
    } chunk_t;
    std::list<chunk_t> chunks;
   public:
    write_file_chunks(void) {
    };
    unsigned long long int eof(void) {
      if(chunks.size() <= 0) return 0;
      return (--(chunks.end()))->end;
    };
    unsigned long long int extends(void) {
      if(chunks.size() <= 0) return 0;
      return chunks.begin()->end;
    };
    void add(unsigned long long int start, unsigned long long int end) {
      chunk_t c;
      c.start = start;
      c.end = end;
      if(chunks.size() <= 0) {
        chunks.push_back(c);
        return;
      };
      for(std::list<chunk_t>::iterator chunk = chunks.begin();
                  chunk != chunks.end();++chunk) {
        if(end < chunk->start) {
          chunks.insert(chunk,c);
          break;
        };
        if(((start >= chunk->start) && (start <= chunk->end)) ||
           ((end >= chunk->start) && (end <= chunk->end))) {
          if(chunk->start < start) start = chunk->start;
          if(chunk->end > end) end = chunk->end;
          chunks.erase(chunk);
          add(start,end);
          break;
        };
      };
    };
  };

  void DataPointFile::write_file() {
    unsigned long long int cksum_p = 0;
    bool do_cksum = (checksums.size() > 0);
    write_file_chunks cksum_chunks;
    for (;;) {
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
      if(do_cksum) {
        cksum_chunks.add(p,p+l);
        if(p == cksum_p) {
          for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                    cksum != checksums.end(); ++cksum) {
            if(*cksum) (*cksum)->add((*(buffer))[h], l);
          }
          cksum_p = p+l;
        } else {
          if(cksum_chunks.extends() > cksum_p) {
            // from file
            if(lseek(fd, cksum_p, SEEK_SET) == cksum_p) {
              const unsigned int tbuf_size = 4096;
              char* tbuf = new char[tbuf_size];
              for(;cksum_chunks.extends() > cksum_p;) {
                unsigned int l = tbuf_size;
                if(l > (cksum_chunks.extends()-cksum_p))
                  l=cksum_chunks.extends()-cksum_p;
                int ll = read(fd,tbuf,l);
                if(ll < 0) { do_cksum=false; break; };
                for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                          cksum != checksums.end(); ++cksum) {
                  if(*cksum) (*cksum)->add(tbuf, ll);
                }
                cksum_p += ll;
              }
              delete tbuf;
            }
          }
        }
      }
      /* 2. write */
      lseek(fd, p, SEEK_SET);
      int l_ = 0;
      int ll = 0;
      while (l_ < l) {
        ll = write(fd, (*(buffer))[h] + l_, l - l_);
        if (ll == -1) { /* error */
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
#ifndef WIN32
    // This is for broken filesystems. Specifically for Lustre.
    if (fsync(fd) != 0 && errno != EINVAL) { // this error is caused by special files like stdout
      logger.msg(ERROR, "fsync of file %s failed: %s", url.Path(), strerror(errno));
      buffer->error_write(true);
    }
#endif
    if (close(fd) != 0) {
      logger.msg(ERROR, "closing file %s failed: %s", url.Path(), strerror(errno));
      buffer->error_write(true);
    }    
    if(do_cksum) {
      for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                cksum != checksums.end(); ++cksum) {
        if(*cksum) (*cksum)->end();
      }
    }
    transfer_cond.signal();
  }

  DataStatus DataPointFile::Check() {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    User user;
    int res = user.check_file_access(url.Path(), O_RDONLY);
    if (res != 0) {
      logger.msg(INFO, "File is not accessible: %s", url.Path());
      return DataStatus::CheckError;
    }
    struct stat st;
    if (stat((url.Path()).c_str(), &st) != 0) {
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
      
    const char* path = url.Path().c_str();
    struct stat st;
    if(stat(path,&st) != 0) {
      if (errno == ENOENT) return DataStatus::Success;
      logger.msg(INFO, "File is not accessible: %s - %s", path, strerror(errno));
      return DataStatus::DeleteError;
    }
    // path is a directory
    if(S_ISDIR(st.st_mode)) {
      if (rmdir(path) == -1) {
        logger.msg(INFO, "Can't delete directory: %s - %s", path, strerror(errno));
        return DataStatus::DeleteError;
      }
      return DataStatus::Success;
    }
    // path is a file
    if(unlink(path) == -1 && errno != ENOENT) {
      logger.msg(INFO, "Can't delete file: %s - %s", path, strerror(errno));
      return DataStatus::DeleteError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StartReading(DataBuffer& buf) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    reading = true;
    /* try to open */
    int flags = O_RDONLY;

    if (url.Path() == "-") // won't work
      fd = dup(STDIN_FILENO);
    else {
      User user;
      if (user.check_file_access(url.Path(), flags) != 0) {
        reading = false;
        return DataStatus::ReadStartError;
      }
      fd = FileOpen(url.Path(), flags);
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
    if(!CreateThreadFunction(&DataPointFile::read_file_start,this)) {
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
    if (buffer->error_read())
      return DataStatus::ReadError;
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StartWriting(DataBuffer& buf,
                                         DataCallback *space_cb) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    writing = true;
    /* try to open */
    buffer = &buf;
    if (url.Path() == "-") { // won't work
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
      User user;
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
      std::string dirpath = Glib::path_get_dirname(url.Path());
      if(dirpath == ".") dirpath = G_DIR_SEPARATOR_S; // shouldn't happen
      if (!DirCreate(dirpath, user.get_uid(), user.get_gid(), S_IRWXU, true)) {
        logger.msg(ERROR, "Failed to create directory %s", dirpath);
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus::WriteStartError;
      }

      /* try to create file, if failed - try to open it */
      int flags = O_WRONLY;
      fd = FileOpen(url.Path(), flags | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
      if (fd == -1)
        fd = FileOpen(url.Path(), flags | O_TRUNC, S_IRUSR | S_IWUSR);
      else  /* this file was created by us. Hence we can set it's owner */
        (fchown(fd, user.get_uid(), user.get_gid()) != 0);
      if (fd == -1) {
        logger.msg(ERROR, "Failed to create/open file %s (%d)", url.Path(), errno);
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus::WriteStartError;
      }

      /* preallocate space */
      buffer->speed.hold(true);
      if (additional_checks && CheckSize()) {
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
    if(!CreateThreadFunction(&DataPointFile::write_file_start,this)) {
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
    // validate file size, if transfer succeeded
    if (!buffer->error() && additional_checks && CheckSize()) {
      struct stat st;
      std::string path = url.Path();
      if (stat(path.c_str(), &st) != 0 && errno != ENOENT) {
        logger.msg(ERROR, "Error during file validation. Can't stat file %s", url.Path());
        return DataStatus::WriteStopError;
      }
      if (errno != ENOENT && GetSize() != st.st_size) {
        logger.msg(ERROR, "Error during file validation: Local file size %llu does not match source file size %llu for file %s",
                          st.st_size, GetSize(), url.Path());
        return DataStatus::WriteStopError;
      }
    }
    
    if (buffer->error_write())
      return DataStatus::WriteError;
    return DataStatus::Success;
  }

  DataStatus DataPointFile::ListFiles(std::list<FileInfo>& files,
                                      bool long_list,
                                      bool /* resolve */,
                                      bool metadata) {
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    std::string dirname = url.Path();
    if (dirname[dirname.length() - 1] == '/')
      dirname.resize(dirname.length() - 1);

    struct stat st;
    if (stat(dirname.c_str(), &st) != 0) {
      logger.msg(INFO, "Failed to read object %s: %s", dirname, strerror(errno));
      return DataStatus::ListError;
    }
    if (S_ISDIR(st.st_mode) && !metadata) {
      try {
        Glib::Dir dir(dirname);
        std::string file_name;
        while ((file_name = dir.read_name()) != "") {
          std::list<FileInfo>::iterator f =
            files.insert(files.end(), FileInfo(file_name.c_str()));
          if (long_list) {
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
      }
      catch (Glib::FileError& e) {
        logger.msg(INFO, "Failed to read object %s", dirname);
        return DataStatus::ListError;
      }
    }
    else {
      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(dirname));
      f->SetMetaData("path", dirname);
      f->SetSize(st.st_size);
      f->SetMetaData("size", tostring(st.st_size));
      logger.msg(INFO, "size is %s", tostring(st.st_size));
      f->SetCreated(st.st_mtime);
      f->SetMetaData("mtime", (Time(st.st_mtime)).str());
    	if (S_ISDIR(st.st_mode)) {
     	  f->SetType(FileInfo::file_type_dir);
        f->SetMetaData("type", "dir");
      }
      else if (S_ISREG(st.st_mode)) {
        f->SetType(FileInfo::file_type_file);
        f->SetMetaData("type", "file");
      }
      // fill some more metadata
      f->SetMetaData("atime", (Time(st.st_atime)).str());
      f->SetMetaData("ctime", (Time(st.st_ctime)).str());
      f->SetMetaData("group", tostring(st.st_gid));
      f->SetMetaData("owner", tostring(st.st_uid));
      unsigned int mode = st.st_mode;
      std::string perms;
      if (mode & S_IRUSR) perms += 'r'; else perms += '-';
      if (mode & S_IWUSR) perms += 'w'; else perms += '-';
      if (mode & S_IXUSR) perms += 'x'; else perms += '-';
#ifndef WIN32
      if (mode & S_IRGRP) perms += 'r'; else perms += '-';
      if (mode & S_IWGRP) perms += 'w'; else perms += '-';
      if (mode & S_IXGRP) perms += 'x'; else perms += '-';
      if (mode & S_IROTH) perms += 'r'; else perms += '-';
      if (mode & S_IWOTH) perms += 'w'; else perms += '-';
      if (mode & S_IXOTH) perms += 'x'; else perms += '-';
#endif
      f->SetMetaData("accessperm", perms);
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

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "file", "HED:DMC", 0, &Arc::DataPointFile::Instance },
  { NULL, NULL, 0, NULL }
};
