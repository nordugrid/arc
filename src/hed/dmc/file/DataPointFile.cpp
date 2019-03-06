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
#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
#include <arc/CheckSum.h>
#include <arc/FileUtils.h>
#include <arc/FileAccess.h>
#include <arc/Utils.h>

#include "DataPointFile.h"

namespace ArcDMCFile {

  using namespace Arc;


  static char const * const stdfds[] = {
    "stdin",
    "stdout",
    "stderr"
  };

  Logger DataPointFile::logger(Logger::getRootLogger(), "DataPoint.File");

  DataPointFile::DataPointFile(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false),
      is_channel(false),
      channel_num(0) {
    fd = -1;
    fa = NULL;
    if (url.Protocol() == "file") {
      cache = false;
      is_channel = false;
      local = true;
    }
    else if (url.Protocol() == "stdio") {
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
    if (((const URL &)(*dmcarg)).Protocol() != "file" && ((const URL &)(*dmcarg)).Protocol() != "stdio")
      return NULL;
    return new DataPointFile(*dmcarg, *dmcarg, dmcarg);
  }

  int DataPointFile::open_channel() {
    // map known channels to strings
    if (!stringto(url.Path().substr(1, url.Path().length()-1), channel_num)) {
      // requested channel is not a number
      if (url.Path() == "/stdin")       channel_num = STDIN_FILENO;
      else if (url.Path() == "/stdout") channel_num = STDOUT_FILENO;
      else if (url.Path() == "/stderr") channel_num = STDERR_FILENO;
      else {
        logger.msg(ERROR, "Unknown channel %s for stdio protocol", url.Path());
        fd = -1;
        return fd;
      }
    }
    int fd = dup(channel_num);
    if (fd == -1) {
      if (channel_num < 3) logger.msg(ERROR, "Failed to open stdio channel %s", stdfds[channel_num]);
      else logger.msg(ERROR, "Failed to open stdio channel %d", channel_num);
    }
    return fd;
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
      if(fd != -1) ::lseek(fd, range_start, SEEK_SET);
      if(fa) fa->fa_lseek(range_start, SEEK_SET);
      offset = range_start;
      if(offset > 0) {
        // Note: checksum calculation not possible
        do_cksum = false;
      }
    }
    else {
      if(fd != -1) lseek(fd, 0, SEEK_SET);
      if(fa) fa->fa_lseek(0, SEEK_SET);
    }
    for (;;) {
      if (limit_length) if (range_length == 0) break;
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
      if (limit_length) if (l > range_length) l = range_length;
      unsigned long long int p = (unsigned long long int)(-1);
      int ll = -1;
      if(fd != -1) {
        p = ::lseek(fd, 0, SEEK_CUR);
        if (p == (unsigned long long int)(-1)) p = offset;
        ll = ::read(fd, (*(buffer))[h], l);
      }
      if(fa) {
        p = fa->fa_lseek(0, SEEK_CUR);
        if (p == (unsigned long long int)(-1)) p = offset;
        ll = fa->fa_read((*(buffer))[h], l);
      }
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
    if(fd != -1) close(fd);
    if(fa) fa->fa_close();
    buffer->eof_read(true);
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
    }
    // which is file EOF according to collected information
    unsigned long long int eof(void) {
      if(chunks.empty()) return 0;
      return (--(chunks.end()))->end;
    }
    // how far non-interruptive file chunk reaches
    unsigned long long int extends(void) {
      if(chunks.empty()) return 0;
      if(chunks.begin()->start > 0) return 0;
      return chunks.begin()->end;
    }
    void add(unsigned long long int start, unsigned long long int end) {
      chunk_t c;
      c.start = start;
      c.end = end;
      if(chunks.empty()) {
        chunks.push_back(c);
        return;
      }
      for(std::list<chunk_t>::iterator chunk = chunks.begin();
                  chunk != chunks.end();++chunk) {
        if(end < chunk->start) {
          chunks.insert(chunk,c);
          return;
        }
        if(((start >= chunk->start) && (start <= chunk->end)) ||
           ((end >= chunk->start) && (end <= chunk->end))) {
          if(chunk->start < start) start = chunk->start;
          if(chunk->end > end) end = chunk->end;
          chunks.erase(chunk);
          add(start,end);
          return;
        }
      }
      chunks.push_back(c);
    }
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
      /* 2. write */
      unsigned int l_ = 0;
      int ll = -1;
      if(fd != -1) {
        off_t coff = lseek(fd, p, SEEK_SET);
        if((coff == p) || is_channel) {
          ll = 0;
          while (l_ < l) {
            ll = write(fd, (*(buffer))[h] + l_, l - l_);
            if (ll == -1) break; // error
            l_ += ll;
          }
        }
      }
      if(fa) {
        off_t coff = fa->fa_lseek(p, SEEK_SET);
        if((coff == p) || is_channel) {
          ll = 0;
          while (l_ < l) {
            ll = fa->fa_write((*(buffer))[h] + l_, l - l_);
            if (ll == -1) break; // error
            l_ += ll;
          }
        }
      }
      if (ll == -1) { // error
        buffer->is_written(h);
        buffer->error_write(true);
        buffer->eof_write(true);
        break;
      }
      /* 2'. checksum */
      if(do_cksum) {
        cksum_chunks.add(p,p+l);
        if(p == cksum_p) {
          for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                    cksum != checksums.end(); ++cksum) {
            if(*cksum) (*cksum)->add((*(buffer))[h], l);
          }
          cksum_p = p+l;
        }
        if(cksum_chunks.extends() > cksum_p) {
          // from file
          off_t coff = 0;
          if(fd != -1) coff = lseek(fd, cksum_p, SEEK_SET);
          if(fa) coff = fa->fa_lseek(cksum_p, SEEK_SET);
          if(coff == cksum_p) {
            const unsigned int tbuf_size = 4096; // too small?
            char* tbuf = new char[tbuf_size];
            for(;cksum_chunks.extends() > cksum_p;) {
              unsigned int l = tbuf_size;
              if(l > (cksum_chunks.extends()-cksum_p)) l=cksum_chunks.extends()-cksum_p;
              int ll = -1;
              if(fd != -1) ll = read(fd,tbuf,l);
              if(fa) ll = fa->fa_read(tbuf,l);
              if(ll < 0) { do_cksum=false; break; }
              for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                        cksum != checksums.end(); ++cksum) {
                if(*cksum) (*cksum)->add(tbuf, ll);
              }
              cksum_p += ll;
            }
            delete[] tbuf;
          }
        }
      }
      /* 3. announce */
      buffer->is_written(h);
    }
    if (fd != -1) {
#ifndef WIN32
      // This is for broken filesystems. Specifically for Lustre.
      if (fsync(fd) != 0 && errno != EINVAL) { // this error is caused by special files like stdout
        logger.msg(ERROR, "fsync of file %s failed: %s", url.Path(), StrError(errno));
        buffer->error_write(true);
      }
#endif
      if(close(fd) != 0) {
        logger.msg(ERROR, "closing file %s failed: %s", url.Path(), StrError(errno));
        buffer->error_write(true);
      }
    }    
    if (fa) {
      // Lustre?
      if(!fa->fa_close()) {
        logger.msg(ERROR, "closing file %s failed: %s", url.Path(), StrError(errno));
        buffer->error_write(true);
      }
    }    
    if((do_cksum) && (cksum_chunks.eof() == cksum_p)) {
      for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                cksum != checksums.end(); ++cksum) {
        if(*cksum) (*cksum)->end();
      }
    }
  }

  DataStatus DataPointFile::Check(bool check_meta) {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsWritingError, EARCLOGIC);
    // check_file_access() is not always correctly evaluating permissions.
    // TODO: redo
    int res = usercfg.GetUser().check_file_access(url.Path(), O_RDONLY);
    if (res != 0) {
      logger.msg(VERBOSE, "File is not accessible: %s", url.Path());
      return DataStatus(DataStatus::CheckError, errno, "File is not accesible "+url.Path());
    }
    if (check_meta) {
      struct stat st;
      if (!FileStat(url.Path(), &st, usercfg.GetUser().get_uid(), usercfg.GetUser().get_gid(), true)) {
        logger.msg(VERBOSE, "Can't stat file: %s: %s", url.Path(), StrError(errno));
        return DataStatus(DataStatus::CheckError, errno, "Failed to stat file "+url.Path());
      }
      SetSize(st.st_size);
      SetModified(st.st_mtime);
    }
    return DataStatus::Success;
  }

  static DataStatus do_stat(const std::string& path, FileInfo& file, DataPoint::DataPointInfoType verb, uid_t uid, gid_t gid) {
    struct stat st;
    if (!FileStat(path, &st, uid, gid, true)) {
      return DataStatus(DataStatus::StatError, errno, "Failed to stat file "+path);
    }
    if(S_ISREG(st.st_mode)) {
      file.SetType(FileInfo::file_type_file);
    } else if(S_ISDIR(st.st_mode)) {
      file.SetType(FileInfo::file_type_dir);
    } else {
      file.SetType(FileInfo::file_type_unknown);
    }
    file.SetSize(st.st_size);
    file.SetModified(st.st_mtime);
    file.SetMetaData("atime", (Time(st.st_atime)).str());
    file.SetMetaData("ctime", (Time(st.st_ctime)).str());
    file.SetMetaData("group", tostring(st.st_gid));
    file.SetMetaData("owner", tostring(st.st_uid));
    std::string perms;
    if (st.st_mode & S_IRUSR) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWUSR) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXUSR) perms += 'x'; else perms += '-';
#ifndef WIN32
    if (st.st_mode & S_IRGRP) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWGRP) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXGRP) perms += 'x'; else perms += '-';
    if (st.st_mode & S_IROTH) perms += 'r'; else perms += '-';
    if (st.st_mode & S_IWOTH) perms += 'w'; else perms += '-';
    if (st.st_mode & S_IXOTH) perms += 'x'; else perms += '-';
#endif
    file.SetMetaData("accessperm", perms);
    return DataStatus::Success;
  }

  DataStatus DataPointFile::Stat(FileInfo& file, DataPointInfoType verb) {

    if(is_channel) {
      int fd = open_channel();
      if (fd == -1){
        logger.msg(VERBOSE, "Can't stat stdio channel %s", url.str());
        return DataStatus(DataStatus::StatError, EBADF, "Can't stat channel");
      }
      struct stat st;
      if(::fstat(fd, &st) != 0) {
        ::close(fd);
        logger.msg(VERBOSE, "Can't stat stdio channel %s", url.str());
        return DataStatus(DataStatus::StatError, EBADF, "Can't stat channel");
      }
      ::close(fd);
      if (channel_num < 3) file.SetName(stdfds[channel_num]);
      else file.SetName(tostring(channel_num));
      file.SetType(FileInfo::file_type_file);
      file.SetMetaData("type", "device");
      file.SetSize(st.st_size);
      file.SetModified(st.st_mtime);
      return DataStatus::Success;
    }

    std::string name = url.Path();
    // to make exact same behaviour for arcls and ngls all
    // lines down to file.SetName(name) should be removed
    // (ngls <local filename> gives full path)
    std::string::size_type p = name.rfind(G_DIR_SEPARATOR);
    while(p != std::string::npos) {
      if(p != (name.length()-1)) {
        name = name.substr(p);
        break;
      }
      name.resize(p);
      p = name.rfind(G_DIR_SEPARATOR);
    }
    // remove first slash
    if(name.find_first_of(G_DIR_SEPARATOR) == 0){
      name = name.substr(name.find_first_not_of(G_DIR_SEPARATOR), name.length()-1);
    }
    file.SetName(name);
    DataStatus res = do_stat(url.Path(), file, verb, usercfg.GetUser().get_uid(), usercfg.GetUser().get_gid());
    if (!res) {
      logger.msg(VERBOSE, "Can't stat file: %s: %s", url.Path(), std::string(res));
      return res;
    }
    SetSize(file.GetSize());
    SetModified(file.GetModified());
    return DataStatus::Success;
  }

  DataStatus DataPointFile::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    FileInfo file;
    DataStatus res = Stat(file, verb);
    if (!res) {
      return DataStatus(DataStatus::ListError, res.GetErrno(), res.GetDesc());
    }
    if(file.GetType() != FileInfo::file_type_dir) {
      logger.msg(WARNING, "%s is not a directory", url.Path());
      return DataStatus(DataStatus::ListError, ENOTDIR, url.Path()+" is not a directory");
    }
    try {
      Glib::Dir dir(url.Path());
      std::string file_name;
      while ((file_name = dir.read_name()) != "") {
        std::string fname = url.Path() + G_DIR_SEPARATOR_S + file_name;
        std::list<FileInfo>::iterator f =
          files.insert(files.end(), FileInfo(file_name.c_str()));
        if (verb & (INFO_TYPE_TYPE | INFO_TYPE_TIMES | INFO_TYPE_CONTENT | INFO_TYPE_ACCESS)) {
          do_stat(fname, *f, verb, usercfg.GetUser().get_uid(), usercfg.GetUser().get_gid());
        }
      }
    } catch (Glib::FileError& e) {
      logger.msg(VERBOSE, "Failed to read object %s: %s", url.Path(), e.what());
      return DataStatus(DataStatus::ListError, "Failed to list directory "+url.Path()+": "+e.what());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::Remove() {
    if (reading) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    if (writing) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
      
    std::string path(url.Path());
    struct stat st;
    if(!FileStat(path, &st, usercfg.GetUser().get_uid(), usercfg.GetUser().get_gid(), true)) {
      logger.msg(VERBOSE, "File is not accessible %s: %s", path, StrError(errno));
      return DataStatus(DataStatus::DeleteError, errno, "Failed to stat file "+path);
    }
    // path is a directory
    if(S_ISDIR(st.st_mode)) {
      if (rmdir(path.c_str()) != 0) {
        logger.msg(VERBOSE, "Can't delete directory %s: %s", path, StrError(errno));
        return DataStatus(DataStatus::DeleteError, errno, "Failed to remove directory "+path);
      }
      return DataStatus::Success;
    }
    // path is a file
    if(!FileDelete(path) && errno != ENOENT) {
      logger.msg(VERBOSE, "Can't delete file %s: %s", path, StrError(errno));
      return DataStatus(DataStatus::DeleteError, errno, "Failed to delete file "+path);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::CreateDirectory(bool with_parents) {
    std::string dirpath = Glib::path_get_dirname(url.Path());
    if(dirpath == ".") dirpath = G_DIR_SEPARATOR_S;

    logger.msg(VERBOSE, "Creating directory %s", dirpath);
    if (!DirCreate(dirpath, S_IRWXU, with_parents)) {
      return DataStatus(DataStatus::CreateDirectoryError, errno, "Failed to create directory "+dirpath);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::Rename(const URL& newurl) {
    logger.msg(VERBOSE, "Renaming %s to %s", url.Path(), newurl.Path());
    if (rename(url.Path().c_str(), newurl.Path().c_str()) != 0) {
      logger.msg(VERBOSE, "Can't rename file %s: %s", url.Path(), StrError(errno));
      return DataStatus(DataStatus::RenameError, errno, "Failed to rename file "+url.Path());
    }
    return DataStatus::Success;
  }


  DataStatus DataPointFile::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    /* try to open */
    int flags = O_RDONLY;
#ifdef WIN32
    flags |= O_BINARY;
#endif
    uid_t uid = usercfg.GetUser().get_uid();
    gid_t gid = usercfg.GetUser().get_gid();

    if (is_channel){
      fa = NULL;
      fd = open_channel();
      if (fd == -1) {
        reading = false;
        return DataStatus(DataStatus::ReadStartError, EBADF, "Channel number is not defined");
      }
    }
    else if(((!uid) || (uid == getuid())) && ((!gid) || (gid == getgid()))) {
      fa = NULL;
      fd = ::open(url.Path().c_str(), flags);
      if (fd == -1) {
        logger.msg(VERBOSE, "Failed to open %s for reading: %s", url.Path(), StrError(errno));
        reading = false;
        return DataStatus(DataStatus::ReadStartError, errno, "Failed to open file "+url.Path()+" for reading");
      }
      /* provide some metadata */
      struct stat st;
      if (::fstat(fd, &st) == 0) {
        SetSize(st.st_size);
        SetModified(st.st_mtime);
      }
    } else {
      fd = -1;
      fa = new FileAccess;
      if(!fa->fa_setuid(uid,gid)) {
        delete fa; fa = NULL;
        logger.msg(VERBOSE, "Failed to switch user id to %d/%d", (unsigned int)uid, (unsigned int)gid);
        reading = false;
        return DataStatus(DataStatus::ReadStartError, EARCUIDSWITCH, "Failed to switch user id to "+tostring(uid)+"/"+tostring(gid));
      }
      if(!fa->fa_open(url.Path(), flags, 0)) {
        delete fa; fa = NULL;
        logger.msg(VERBOSE, "Failed to create/open file %s: %s", url.Path(), StrError(errno));
        reading = false;
        return DataStatus(DataStatus::ReadStartError, errno, "Failed to open file "+url.Path()+" for reading");
      }
      struct stat st;
      if(fa->fa_fstat(st)) {
        SetSize(st.st_size);
        SetModified(st.st_mtime);
      }
    }
    buffer = &buf;
    /* create thread to maintain reading */
    if(!CreateThreadFunction(&DataPointFile::read_file_start,this,&transfers_started)) {
      if(fd != -1) ::close(fd);
      if(fa) { fa->fa_close(); delete fa; }
      fd = -1; fa = NULL;
      logger.msg(VERBOSE, "Failed to create thread");
      reading = false;
      return DataStatus(DataStatus::ReadStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StopReading() {
    if (!reading) return DataStatus(DataStatus::ReadStopError, EARCLOGIC, "Not reading");
    reading = false;
    if (!buffer->eof_read()) {
      buffer->error_read(true);      /* trigger transfer error */
      if(fd != -1) ::close(fd);
      if(fa) fa->fa_close(); // protect?
      fd = -1;
    }
    // buffer->wait_eof_read();
    transfers_started.wait();         /* wait till reading thread exited */
    delete fa; fa = NULL;
    // TODO: error description from reading thread
    if (buffer->error_read()) return DataStatus::ReadError;
    return DataStatus::Success;
  }

  static bool file_allocate(int fd, FileAccess* fa, unsigned long long int& fsize) {
    if(fa) {
      off_t nsize = fa->fa_fallocate(fsize);
      if((fa->geterrno() == 0) || (fa->geterrno() == ENOSPC)) {
        fsize = nsize;
        return true;
      }
      return false;
    }
#ifdef HAVE_POSIX_FALLOCATE
    int err = posix_fallocate(fd,0,fsize);
    if((err == 0) || (err == ENOSPC)) {
      fsize = lseek(fd,0,SEEK_END);
      return true;
    }
    return false;
#else
    unsigned long long int old_size = lseek(fd, 0, SEEK_END);
    if(old_size >= fsize) { fsize = old_size; return true; }
    char buf[65536];
    memset(buf, 0xFF, sizeof(buf));
    while(old_size < fsize) {
      size_t l = sizeof(buf);
      if (l > (fsize - old_size)) l = fsize - old_size;
      // because filesytem can skip empty blocks do real write 
      if (write(fd, buf, l) == -1) {
        fsize = old_size; return (errno = ENOSPC);
      }
      old_size = lseek(fd, 0, SEEK_END);
    }
    fsize = old_size; return true;
#endif
  }

  DataStatus DataPointFile::StartWriting(DataBuffer& buf, DataCallback *space_cb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    writing = true;
    uid_t uid = usercfg.GetUser().get_uid();
    gid_t gid = usercfg.GetUser().get_gid();
    /* try to open */
    buffer = &buf;
    if (is_channel) {
      fd = open_channel();
      if (fd == -1) {
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus(DataStatus::WriteStartError, EBADF, "Channel number is not defined");
      }
    }
    else {
      /* do not check permissions to create anything here -
         suppose it path was checked at higher level */
      /* make directories */
      if (url.Path().empty()) {
        logger.msg(VERBOSE, "Invalid url: %s", url.str());
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus(DataStatus::WriteStartError, EINVAL, "Invalid URL "+url.str());
      }
      std::string dirpath = Glib::path_get_dirname(url.Path());
      if(dirpath == ".") dirpath = G_DIR_SEPARATOR_S; // shouldn't happen
      if (!DirCreate(dirpath, uid, gid, S_IRWXU, true)) {
        logger.msg(VERBOSE, "Failed to create directory %s: %s", dirpath, StrError(errno));
        buffer->error_write(true);
        buffer->eof_write(true);
        writing = false;
        return DataStatus(DataStatus::WriteStartError, errno, "Failed to create directory "+dirpath);
      }

      /* try to create file. Opening an existing file will cause failure */
      int flags = (checksums.size() > 0)?O_RDWR:O_WRONLY;
#ifdef WIN32
      flags |= O_BINARY;
#endif
      if(((!uid) || (uid == getuid())) && ((!gid) || (gid == getgid()))) {
        fa = NULL;
        fd = ::open(url.Path().c_str(), flags | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd == -1) {
          logger.msg(VERBOSE, "Failed to create file %s: %s", url.Path(), StrError(errno));
          buffer->error_write(true);
          buffer->eof_write(true);
          writing = false;
          return DataStatus(DataStatus::WriteStartError, errno, "Failed to create file "+url.Path());
        }
      } else {
        fd = -1;
        fa = new FileAccess;
        if(!fa->fa_setuid(uid,gid)) {
          delete fa; fa = NULL;
          logger.msg(VERBOSE, "Failed to switch user id to %d/%d", (unsigned int)uid, (unsigned int)gid);
          buffer->error_write(true);
          buffer->eof_write(true);
          writing = false;
          return DataStatus(DataStatus::WriteStartError, EARCUIDSWITCH, "Failed to switch user id to "+tostring(uid)+"/"+tostring(gid));
        }
        if(!fa->fa_open(url.Path(), flags | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) {
          delete fa; fa = NULL;
          logger.msg(VERBOSE, "Failed to create file %s: %s", url.Path(), StrError(errno));
          buffer->error_write(true);
          buffer->eof_write(true);
          writing = false;
          return DataStatus(DataStatus::WriteStartError, errno, "Failed to create file "+url.Path());
        }
      }

      /* preallocate space */
      buffer->speed.hold(true);
      if (additional_checks && CheckSize() && GetSize() > 0) {
        unsigned long long int fsize = GetSize();
        logger.msg(INFO, "setting file %s to size %llu", url.Path(), fsize);
        while(true) {
          unsigned long long int nsize = fsize;
          if(file_allocate(fd,fa,nsize)) {
            if(nsize >= fsize) break;
            /* out of space */
            if (space_cb != NULL) {
              if (space_cb->cb(fsize-nsize)) continue;
            }
          }
          if(fd != -1) {
            (lseek(fd, 0, SEEK_SET) == -1);
            (ftruncate(fd, 0) != 0);
            (close(fd) != -1); fd = -1;
          }
          if(fa) {
            fa->fa_lseek(0, SEEK_SET);
            fa->fa_ftruncate(0);
            fa->fa_close(); delete fa; fa = NULL;
          }
          logger.msg(VERBOSE, "Failed to preallocate space for %s", url.Path());
          buffer->speed.reset();
          buffer->speed.hold(false);
          buffer->error_write(true);
          buffer->eof_write(true);
          writing = false;
          return DataStatus(DataStatus::WriteStartError, ENOSPC, "Failed to preallocate space for file "+url.Path());
        }
      }
    }
    buffer->speed.reset();
    buffer->speed.hold(false);
    /* create thread to maintain writing */
    if(!CreateThreadFunction(&DataPointFile::write_file_start,this,&transfers_started)) {
      if(fd != -1) close(fd); fd = -1;
      if(fa) fa->fa_close(); delete fa; fa = NULL;
      buffer->error_write(true);
      buffer->eof_write(true);
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointFile::StopWriting() {
    if (!writing) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");
    writing = false;
    if (!buffer->eof_write()) {
      buffer->error_write(true);      /* trigger transfer error */
      if(fd != -1) close(fd);
      if(fa) fa->fa_close();
      fd = -1;
    }
    // buffer->wait_eof_write();
    transfers_started.wait();         /* wait till writing thread exited */

    // clean up if transfer failed for any reason
    if (buffer->error()) {
      bool err = false;
      if (fa) err = fa->fa_unlink(url.Path());
      else err = FileDelete(url.Path());
      if (!err && errno != ENOENT) logger.msg(WARNING, "Failed to clean up file %s: %s", url.Path(), StrError(errno));
    }
    delete fa; fa = NULL;

    // validate file size, if transfer succeeded
    if (!buffer->error() && additional_checks && CheckSize() && !is_channel) {
      struct stat st;
      std::string path = url.Path();
      if (!FileStat(path, &st, usercfg.GetUser().get_uid(), usercfg.GetUser().get_gid(), true)) {
        logger.msg(VERBOSE, "Error during file validation. Can't stat file %s: %s", url.Path(), StrError(errno));
        return DataStatus(DataStatus::WriteStopError, errno, "Failed to stat result file "+url.Path());
      }
      if (GetSize() != st.st_size) {
        logger.msg(VERBOSE, "Error during file validation: Local file size %llu does not match source file size %llu for file %s",
                          st.st_size, GetSize(), url.Path());
        return DataStatus(DataStatus::WriteStopError, "Local file size does not match source file for "+url.Path());
      }
    }
    
    // TODO: error description from writing thread
    if (buffer->error_write()) return DataStatus::WriteError;
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

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "file", "HED:DMC", "Regular local file", 0, &ArcDMCFile::DataPointFile::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
