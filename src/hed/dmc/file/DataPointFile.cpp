#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/data/MkDirRecursive.h>
#include <arc/data/CheckFile.h>
#include <arc/Logger.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataCallback.h>
#include "DataPointFile.h"

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cerrno>

namespace Arc {

Logger DataPointFile::logger(DataPoint::logger, "File");

DataPointFile::DataPointFile(const URL& url) : DataPointDirect(url) {}

DataPointFile::~DataPointFile(void) {
  stop_reading();
  stop_writing();
  deinit_handle();
}

bool DataPointFile::init_handle(void) {
  if(!DataPointDirect::init_handle()) return false;
  if(url.Protocol() == "file") {
    cacheable=false; is_channel=false;
  } else if(url.Path() == "-") {
    cacheable=false; linkable=false; is_channel=true;
  } else {
    return false;
  };
  return true;
}

bool DataPointFile::deinit_handle(void) {
  if(!DataPointDirect::deinit_handle()) return false;
  return true;
}

void* DataPointFile::read_file(void* arg) {
  DataPointFile* it = (DataPointFile*)arg;
  bool limit_length = false;
  unsigned long long int range_length;
  unsigned long long int offset = 0;
  if(it->range_end > it->range_start) {
    range_length = it->range_end - it->range_start;
    limit_length = true;
    lseek(it->fd,it->range_start,SEEK_SET);
    offset=it->range_start;
  } else {
    lseek(it->fd,0,SEEK_SET);
  };
  for(;;) {
    if(limit_length) if(range_length==0) break;
  /* read from fd here and push to buffer */
  /* 1. claim buffer */
    int h;
    unsigned int l;
    if(!it->buffer->for_read(h,l,true)) { 
      /* failed to get buffer - must be error or request to exit */
      it->buffer->error_read(true);
      break;
    };
    if(it->buffer->error()) {
      it->buffer->is_read(h,0,0);
      break;
    };
  /* 2. read */
    if(limit_length) if(l>range_length) l=range_length;
    unsigned long long int p = lseek(it->fd,0,SEEK_CUR);
    if(p == (unsigned long long int)(-1)) p=offset;
    int ll = read(it->fd,(*(it->buffer))[h],l);
    if(ll == -1) { /* error */
      it->buffer->is_read(h,0,0);
      it->buffer->error_read(true);
      break;
    };
    if(ll == 0) { /* eof */
      it->buffer->is_read(h,0,0);
      break;
    };
  /* 3. announce */
    it->buffer->is_read(h,ll,p);
    if(limit_length) {
      if(ll>range_length) { range_length=0; } else { range_length-=ll; };
    };
    offset+=ll; // for non-seakable files
  };
  close(it->fd);
  it->buffer->eof_read(true);
  /* inform parent thread about exit */
  it->file_thread_exited.signal();
  return NULL;
}

void* DataPointFile::write_file(void* arg) {
  DataPointFile* it = (DataPointFile*)arg;
  for(;;) {
  /* take from buffer and write to fd */
  /* 1. claim buffer */
    int h;
    unsigned int l;
    unsigned long long int p;
    if(!it->buffer->for_write(h,l,p,true)) {
      /* failed to get buffer - must be error or request to exit */
      if(!it->buffer->eof_read()) it->buffer->error_write(true);
      it->buffer->eof_write(true);
      break;
    };
    if(it->buffer->error()) {
      it->buffer->is_written(h);
      it->buffer->eof_write(true);
      break;
    };
  /* 2. write */
    lseek(it->fd,p,SEEK_SET);
    int l_=0;
    int ll = 0;
    for(;l_<l;) {
      ll = write(it->fd,(*(it->buffer))[h]+l_,l-l_);
      if(ll == -1) { /* error */
        it->buffer->is_written(h);
        it->buffer->error_write(true);
        it->buffer->eof_write(true);
        break;
      };
      l_+=ll;
    };
    if(ll == -1) break;
  /* 3. announce */
    it->buffer->is_written(h);
  };
  close(it->fd);
  /* inform parent thread about exit */
  it->file_thread_exited.signal();
  return NULL;
}

bool DataPointFile::check(void) {
  if(!DataPointDirect::check()) return false;
  int res=check_file_access(url.Path(),O_RDONLY,get_user_id(),(gid_t)(-1));
  if(res != 0) {
    logger.msg(INFO, "File is not accessible: %s", url.Path().c_str());
    return false;
  };
  struct stat st;
  if(stat(url.Path().c_str(),&st) != 0) {
    logger.msg(INFO, "Can't stat file: %s", url.Path().c_str());
    return false;
  };
  SetSize(st.st_size);
  SetCreated(st.st_mtime);
  return true;
}

bool DataPointFile::remove(void) {
  if(!DataPointDirect::remove()) return false;
  if(unlink(url.Path().c_str()) == -1) {
    if(errno != ENOENT) return false;
  };
  return true;
}

bool DataPointFile::start_reading(DataBufferPar &buf) {
  if(!DataPointDirect::start_reading(buf)) return false;
  /* try to open */
  file_thread_exited.reset();
  if(url.Path() == "-") {
    fd=dup(STDIN_FILENO);
  } else {
    if(check_file_access(url.Path(),O_RDONLY,
                       get_user_id(),(gid_t)(-1)) != 0) {
      DataPointDirect::stop_reading();
      return false;
    };
    fd=open(url.Path().c_str(),O_RDONLY);
  };
  if(fd == -1) {
    DataPointDirect::stop_reading();
    return false;
  };
  /* provide some metadata */
  struct stat st;
  if(fstat(fd,&st) == 0) {
    SetSize(st.st_size);
    SetCreated(st.st_mtime);
  };
  buffer=&buf;
  /* create thread to maintain reading */
  pthread_attr_init(&file_thread_attr);
  pthread_attr_setdetachstate(&file_thread_attr,PTHREAD_CREATE_DETACHED);
  if(pthread_create(&file_thread,&file_thread_attr,&read_file,this) != 0) {
    pthread_attr_destroy(&file_thread_attr);
    close(fd); fd=-1; 
    DataPointDirect::stop_reading();
    return false;
  };
  return true;
}

bool DataPointFile::stop_reading(void) {
  if(!DataPointDirect::stop_reading()) return false;
  if(!buffer->eof_read()) {
    buffer->error_read(true);        /* trigger transfer error */
    close(fd); fd=-1;
  };
  file_thread_exited.wait();  /* wait till reading thread exited */
  pthread_attr_destroy(&file_thread_attr);
  return true;
}

bool DataPointFile::start_writing(DataBufferPar &buf,DataCallback *space_cb) {
  if(!DataPointDirect::start_writing(buf,space_cb)) return false;
  /* try to open */
  file_thread_exited.reset();
  buffer=&buf;
  if(url.Path() == "-") {
    fd=dup(STDOUT_FILENO);
    if(fd == -1) {
      logger.msg(ERROR, "Failed to use channel stdout");
      buffer->error_write(true); buffer->eof_write(true);
      DataPointDirect::stop_writing();
      return false;
    };
  } else {
    uid_t uid=get_user_id();
    gid_t gid=get_user_group(uid);
    /* do not check permissions to create anything here - 
       suppose it path was checked at higher level */
    /* make directories */
    if(url.Path().empty()) {
      logger.msg(ERROR, "Invalid url: %s", url.str().c_str());
      buffer->error_write(true); buffer->eof_write(true);
      DataPointDirect::stop_writing();
      return false;
    };
    {
      std::string dirpath = url.Path();
      int n = dirpath.rfind('/');
      if(n == 0) {
        dirpath="/";
      } else {
        dirpath.erase(n,dirpath.length()-n+1);
      };
      if(mkdir_recursive("",dirpath,S_IRWXU,uid,gid) != 0) {
        if(errno != EEXIST) {
          logger.msg(ERROR, "Failed to create/find directory %s", dirpath.c_str());
          buffer->error_write(true); buffer->eof_write(true);
          DataPointDirect::stop_writing();
          return false;
        };
      }; 
    };
    /* try to create file, if failed - try to open it */
    fd=open(url.Path().c_str(),O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
    if(fd == -1) {
      fd=open(url.Path().c_str(),O_WRONLY | O_TRUNC,S_IRUSR | S_IWUSR);
    }
    else { /* this file was created by us. Hence we can set it's owner */
      fchown(fd,uid,gid);
    };
    if(fd == -1) {
      logger.msg(ERROR, "Failed to create/open file %s", url.Path().c_str());
      buffer->error_write(true); buffer->eof_write(true);
      DataPointDirect::stop_writing();
      return false;
    };

    /* preallocate space */
    buffer->speed.hold(true);
    unsigned long long int fsize = GetSize();
    if(fsize > 0) {
      logger.msg(INFO, "setting file %s to size %llu", url.Path().c_str(), fsize);
      /* because filesytem can skip empty blocks do real write */
      unsigned long long int old_size=lseek(fd,0,SEEK_END);
      if(old_size < fsize) {
        char buf[65536];
        memset(buf,0xFF,sizeof(buf));
        unsigned int l=1;
        for(;l>0;) {
          old_size=lseek(fd,0,SEEK_END);
          l=sizeof(buf);
          if(l > (fsize-old_size)) l=fsize-old_size;
          if(write(fd,buf,l) == -1) {
            /* out of space */
            perror("write"); /* not good, but let it be */
            if(space_cb != NULL) {
              if(space_cb->cb((unsigned long long int)l)) continue;
            };
            lseek(fd,0,SEEK_SET); 
            ftruncate(fd,0);
            close(fd); fd=-1; 
            logger.msg(INFO, "Failed to preallocate space");
            buffer->speed.reset();
            buffer->speed.hold(false);
            buffer->error_write(true); buffer->eof_write(true);
            DataPointDirect::stop_writing();
            return false;
          };
        };
      };
    };
  };
  buffer->speed.reset();
  buffer->speed.hold(false);
  /* create thread to maintain writing */
  pthread_attr_init(&file_thread_attr);
  pthread_attr_setdetachstate(&file_thread_attr,PTHREAD_CREATE_DETACHED);
  if(pthread_create(&file_thread,&file_thread_attr,&write_file,this) != 0) {
    pthread_attr_destroy(&file_thread_attr);
    close(fd); fd=-1;
    buffer->error_write(true); buffer->eof_write(true);
    DataPointDirect::stop_writing();
    return false;
  };
  return true;
}

bool DataPointFile::stop_writing(void) {
  if(!DataPointDirect::stop_writing()) return false;
  if(!buffer->eof_write()) {
    buffer->error_write(true);        /* trigger transfer error */
    close(fd); fd=-1;
  };
  file_thread_exited.wait();  /* wait till reading thread exited */
  pthread_attr_destroy(&file_thread_attr);
  return true;
}

bool DataPointFile::list_files(std::list<FileInfo> &files,bool resolve) {
  if(!DataPointDirect::list_files(files,resolve)) return false;
  std::string dirname = url.Path();
  if(dirname[dirname.length()-1] == '/') dirname.resize(dirname.length()-1);
  DIR *dir=opendir(dirname.c_str());
  if(dir == NULL) {
    // maybe a file
    struct stat st;
    std::list<FileInfo>::iterator f = 
               files.insert(files.end(),FileInfo(dirname));
    if(stat(dirname.c_str(),&st) == 0) {
      f->SetSize(st.st_size);
      f->SetCreated(st.st_mtime);
      if(S_ISDIR(st.st_mode)) {
        f->SetType(FileInfo::file_type_dir);
      } else if(S_ISREG(st.st_mode)) {
        f->SetType(FileInfo::file_type_file);
      };
      return true;
    };
    files.erase(f);
    logger.msg(INFO, "Failed to read object: %s", dirname.c_str());
    return false;
  };
  for(;;) {
    struct dirent file_;
    struct dirent *file;
    readdir_r(dir,&file_,&file);
    if(file == NULL) break;
    if(file->d_name[0] == '.') {
      if(file->d_name[1] == 0) continue;
      if(file->d_name[1] == '.') if(file->d_name[2] == 0) continue;
    };
    std::list<FileInfo>::iterator f = 
               files.insert(files.end(),FileInfo(file->d_name));
    if(resolve) {
      std::string fname =  dirname+"/"+file->d_name;
      struct stat st;
      if(stat(fname.c_str(),&st) == 0) {
        f->SetSize(st.st_size);
        f->SetCreated(st.st_mtime);
        if(S_ISDIR(st.st_mode)) {
          f->SetType(FileInfo::file_type_dir);
        } else if(S_ISREG(st.st_mode)) {
          f->SetType(FileInfo::file_type_file);
        };
      };
    };
  }; 
  return true;
}

bool DataPointFile::analyze(analyze_t &arg) {
  if(!DataPointDirect::analyze(arg)) return false;
  if(url.Path() == "-") { arg.cache=false; arg.readonly=false; };
  if(url.Protocol() == "file") {
    arg.local=true; arg.cache=false;
  };
  return true;
}

bool DataPointFile::out_of_order(void) {
  if(!url) return false;
  if(url.Protocol() == "file") return true;
  return false;
}

} // namespace Arc
