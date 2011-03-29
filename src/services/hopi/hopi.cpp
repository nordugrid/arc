#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <arc/message/MessageAttributes.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/FileUtils.h>


#include "hopi.h"
#include "PayloadFile.h"

namespace Hopi {

static Arc::Plugin *get_service(Arc::PluginArgument* arg)
{
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new Hopi((Arc::Config*)(*srvarg));
}

Arc::Logger Hopi::logger(Arc::Logger::rootLogger, "Hopi");

class HopiFileChunks {
 private:
  static std::map<std::string,HopiFileChunks> files;
  static Glib::Mutex lock;
  static int timeout;
  static time_t last_timeout;
  typedef std::list<std::pair<off_t,off_t> > chunks_t;
  chunks_t chunks;
  off_t size;
  time_t last_accessed;
  int refcount;
  std::map<std::string,HopiFileChunks>::iterator self;
  HopiFileChunks(void);
 public:
  static void Timeout(int t) { timeout=t; };
  void Add(off_t start,off_t end);
  off_t Size(void) { return size; };
  void Size(off_t size) {
    lock.lock();
    if(size > HopiFileChunks::size) HopiFileChunks::size = size;
    lock.unlock();
  };
  std::string Path(void) { return self->first; };
  static HopiFileChunks& Get(std::string path);
  static HopiFileChunks* GetStuck(void);
  static HopiFileChunks* GetFirst(void);
  void Remove(void);
  void Release(void);
  bool Complete(void);
  void Print(void);
};

std::map<std::string,HopiFileChunks> HopiFileChunks::files;
Glib::Mutex HopiFileChunks::lock;
int HopiFileChunks::timeout = 600; // 10 minutes by default
time_t HopiFileChunks::last_timeout = time(NULL);

void HopiFileChunks::Print(void) {
  int n = 0;
  for(chunks_t::iterator c = chunks.begin();c!=chunks.end();++c) {
    Hopi::logger.msg(Arc::DEBUG, "Chunk %u: %u - %u",n,c->first,c->second);
  };
}

HopiFileChunks::HopiFileChunks(void):size(0),last_accessed(time(NULL)),refcount(0),self(files.end()) {
}

HopiFileChunks* HopiFileChunks::GetStuck(void) {
  if(((int)(time(NULL)-last_timeout)) < timeout) return NULL;
  lock.lock();
  for(std::map<std::string,HopiFileChunks>::iterator f = files.begin();
                    f != files.end();++f) {
    if((f->second.refcount <= 0) && 
       (((int)(time(NULL) - f->second.last_accessed)) >= timeout )) {
      ++(f->second.refcount);
      lock.unlock();
      return &(f->second);
    }
  }
  last_timeout=time(NULL);
  lock.unlock();
  return NULL;
}

HopiFileChunks* HopiFileChunks::GetFirst(void) {
  lock.lock();
  std::map<std::string,HopiFileChunks>::iterator f = files.begin();
  if(f != files.end()) {
    ++(f->second.refcount);
    lock.unlock();
    return &(f->second);
  };
  lock.unlock();
  return NULL;
}

void HopiFileChunks::Remove(void) {
  lock.lock();
  --refcount;
  if(refcount <= 0) {
    if(self != files.end()) {
      files.erase(self);
    }
  }
  lock.unlock();
}

HopiFileChunks& HopiFileChunks::Get(std::string path) {
  lock.lock();
  std::map<std::string,HopiFileChunks>::iterator c = files.find(path);
  if(c == files.end()) {
    c=files.insert(std::pair<std::string,HopiFileChunks>(path,HopiFileChunks())).first;
    c->second.self=c;
  }
  ++(c->second.refcount);
  lock.unlock();
  return (c->second);
}

void HopiFileChunks::Release(void) {
  lock.lock();
  if(chunks.empty()) {
    lock.unlock();
    Remove();
  } else {
    --refcount;
    lock.unlock();
  }
}

void HopiFileChunks::Add(off_t start,off_t end) {
  lock.lock();
  last_accessed=time(NULL);
  if(end > size) size=end;
  for(chunks_t::iterator chunk = chunks.begin();chunk!=chunks.end();++chunk) {
    if((start >= chunk->first) && (start <= chunk->second)) {
      // New chunk starts within existing chunk
      if(end > chunk->second) {
        // Extend chunk
        chunk->second=end;
        // Merge overlapping chunks
        chunks_t::iterator chunk_ = chunk;
        ++chunk_;
        for(;chunk_!=chunks.end();) {
          if(chunk->second < chunk_->first) break;
          // Merge two chunks
          if(chunk_->second > chunk->second) chunk->second=chunk_->second;
          chunk_=chunks.erase(chunk_);
        };
      };
      lock.unlock();
      return;
    } else if((end >= chunk->first) && (end <= chunk->second)) {
      // New chunk ends within existing chunk
      if(start < chunk->first) {
        // Extend chunk
        chunk->first=start;
      };
      lock.unlock();
      return;
    } else if(end < chunk->first) {
      // New chunk is between existing chunks or first chunk
      chunks.insert(chunk,std::pair<off_t,off_t>(start,end));
      lock.unlock();
      return;
    };
  };
  // New chunk is last chunk or there are no chunks currently
  chunks.insert(chunks.end(),std::pair<off_t,off_t>(start,end));
  lock.unlock();
}

bool HopiFileChunks::Complete(void) {
  lock.lock();
  bool r = ((chunks.size() == 1) &&
            (chunks.begin()->first == 0) &&
            (chunks.begin()->second == size));
  lock.unlock();
  return r;
}

class HopiFile {
  int handle;
  std::string path;
  bool for_read;
  bool slave;
  HopiFileChunks& chunks;
 public:
  HopiFile(const std::string& path,bool for_read,bool slave);
  ~HopiFile(void);
  int Write(void* buf,off_t offset,size_t size);
  int Read(void* buf,off_t offset,size_t size);
  int Write(off_t offset,size_t size);
  int Read(off_t offset,size_t size);
  void Size(off_t size) { chunks.Size(size); };
  off_t Size(void) { return chunks.Size(); };
  operator bool(void) { return (handle != -1); };
  bool operator!(void) { return (handle == -1); };
  void Destroy(void);
  static void DestroyStuck(void);
  static void DestroyAll(void);
};

HopiFile::HopiFile(const std::string& path,bool for_read,bool slave):handle(-1),chunks(HopiFileChunks::Get(path)) {
  HopiFile::for_read=for_read;
  HopiFile::slave=slave;
  HopiFile::path=path;
  if(for_read) {
    handle=::open(path.c_str(),O_RDONLY);
  } else {
    if(slave) {
      handle=::open(path.c_str(),O_WRONLY);
      if(handle == -1) {
        if(errno == ENOENT) {
          Hopi::logger.msg(Arc::ERROR, "Hopi SlaveMode is active, PUT is only allowed to existing files");
        }
      }
    } else {
      handle=::open(path.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    }
  }
  if(handle == -1) {
    Hopi::logger.msg(Arc::ERROR, Arc::StrError(errno));
  }
}
  
HopiFile::~HopiFile(void) {
  if(handle != -1) {
    close(handle);
    if(!for_read) {
      if(chunks.Complete()) {
        if(slave) {
          Hopi::logger.msg(Arc::VERBOSE, "Removing complete file in slave mode");
          ::unlink(path.c_str());
        }
        chunks.Remove();
        return;
      }
    }
  }
  chunks.Release();
}

void HopiFile::Destroy(void) {
  if(handle != -1) close(handle);
  handle=-1;
  ::unlink(path.c_str());
  chunks.Remove();
}

void HopiFile::DestroyStuck(void) {
  std::string prev_path;
  for(;;) {
    HopiFileChunks* stuck_chunks = HopiFileChunks::GetStuck();
    if(!stuck_chunks) return;
    std::string stuck_path = stuck_chunks->Path();
    if(stuck_path == prev_path) {
      // This may happen if other thread just started accessing this file
      stuck_chunks->Release();
      return;
    }
    ::unlink(stuck_path.c_str());
    stuck_chunks->Remove();
    prev_path=stuck_path;
  }
}

void HopiFile::DestroyAll(void) {
  std::string prev_path;
  for(;;) {
    HopiFileChunks* first_chunks = HopiFileChunks::GetFirst();
    if(!first_chunks) return;
    std::string first_path = first_chunks->Path();
    if(first_path == prev_path) {
      // This may happen if other thread just started accessing this file
      first_chunks->Release();
      return;
    }
    ::unlink(first_path.c_str());
    first_chunks->Remove();
    prev_path=first_path;
  }
}

int HopiFile::Write(void* buf,off_t offset,size_t size) {
  if(handle == -1) return -1;
  if(for_read) return -1;
  int s = size;
  if(lseek(handle,offset,SEEK_SET) != offset) return 0;
  for(;s>0;) {
    ssize_t l = write(handle,buf,s);
    if(l == -1) return -1;
    chunks.Add(offset,offset+l);
    chunks.Print();
    s-=l; buf=((char*)buf)+l; offset+=l;
  }
  return size;
}

int HopiFile::Write(off_t offset,size_t size) {
  if(handle == -1) return -1;
  if(for_read) return -1;
  chunks.Add(offset,offset+size);
  chunks.Print();
  return size;
}

int HopiFile::Read(void* buf,off_t offset,size_t size) {
  if(handle == -1) return -1;
  if(!for_read) return -1;
  if(lseek(handle,offset,SEEK_SET) != offset) return 0;
  return read(handle,buf,size);
}

int HopiFile::Read(off_t /* offset */,size_t size) {
  if(handle == -1) return -1;
  if(!for_read) return -1;
  return size;
}

class HopiFileTimeout {
 private:
  static std::map<std::string,time_t> files;
  static Glib::Mutex lock;
  static int timeout;
  std::string path;
 public:
  HopiFileTimeout(const std::string& p);
  ~HopiFileTimeout(void);
  static void Timeout(int t) { timeout=t; };
  void Destroy(void);
  static void Add(const std::string& p);
  static void DestroyOld(void);
  static void DestroyAll(void);
};

std::map<std::string,time_t> HopiFileTimeout::files;
Glib::Mutex HopiFileTimeout::lock;
int HopiFileTimeout::timeout = 10;

HopiFileTimeout::HopiFileTimeout(const std::string& p):path(p) {
  lock.lock();
  files[path]=time(NULL);
  lock.unlock();
}

HopiFileTimeout::~HopiFileTimeout(void) {
}

void HopiFileTimeout::Destroy(void) {
  lock.lock();
  std::map<std::string,time_t>::iterator f = files.find(path);
  if(f != files.end()) files.erase(f);
  lock.unlock();
  ::unlink(path.c_str());
}

void HopiFileTimeout::Add(const std::string& p) {
  lock.lock();
  files[p]=time(NULL);
  lock.unlock();
}

void HopiFileTimeout::DestroyOld(void) {
  lock.lock();
  std::map<std::string,time_t>::iterator f = files.begin();
  for(;f != files.end();) {
    int delta = (unsigned int)(time(NULL) - f->second);
    if(delta >= timeout) {
      ::unlink(f->first.c_str());
      std::map<std::string,time_t>::iterator f_ = f;
      ++f;
      files.erase(f_);
      continue;
    };
    ++f;
  }
  lock.unlock();
}

void HopiFileTimeout::DestroyAll(void) {
  lock.lock();
  std::map<std::string,time_t>::iterator f = files.begin();
  for(;f != files.end();) {
    ::unlink(f->first.c_str());
    std::map<std::string,time_t>::iterator f_ = f;
    ++f;
    files.erase(f_);
  }
  lock.unlock();
}


Hopi::Hopi(Arc::Config *cfg):RegisteredService(cfg),slave_mode(false)
{
    logger.msg(Arc::INFO, "Hopi Initialized"); 
    doc_root = (std::string)((*cfg)["DocumentRoot"]);
    if (doc_root.empty()) {
        doc_root = "./";
    }
    logger.msg(Arc::INFO, "Hopi DocumentRoot is " + doc_root);
    slave_mode = (((std::string)((*cfg)["SlaveMode"])) == "1" || ((std::string)((*cfg)["SlaveMode"])) == "yes");
    if (slave_mode) logger.msg(Arc::INFO, "Hopi SlaveMode is on!");
    int timeout;
    if(Arc::stringto((std::string)((*cfg)["UploadTimeout"]),timeout)) {
        if(timeout > 0) HopiFileChunks::Timeout(timeout);
    }
    if(Arc::stringto((std::string)((*cfg)["DownloadTimeout"]),timeout)) {
        if(timeout > 0) HopiFileTimeout::Timeout(timeout);
    }
    uint64_t threshold;
    if(Arc::stringto((std::string)((*cfg)["MemoryMapThreshold"]),threshold)) {
      if(threshold > 0) PayloadBigFile::Threshold(threshold);
    }
}

bool Hopi::RegistrationCollector(Arc::XMLNode &doc) {
    Arc::NS isis_ns; isis_ns["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
    Arc::XMLNode regentry(isis_ns, "RegEntry");
    regentry.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.storage.hopi";
    regentry.New(doc);
    return true;
}

Hopi::~Hopi(void)
{
    logger.msg(Arc::INFO, "Hopi shutdown");
    HopiFile::DestroyAll();
    HopiFileTimeout::DestroyAll();
}

Arc::MessagePayload *Hopi::Get(const std::string &path, const std::string &base_url, unsigned long long int range_start, unsigned long long int range_end)
{
    // XXX eliminate relative paths first
    std::string full_path = Glib::build_filename(doc_root, path);
    if (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == true) {
        if (Glib::file_test(full_path, Glib::FILE_TEST_IS_REGULAR) == true) {
            // register file for removal after timeout
            Arc::MessagePayload * pf = newFileRead(full_path.c_str(),range_start,range_end);
            if(pf && slave_mode) HopiFileTimeout::Add(full_path);
            return pf;
        } else if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR) && !slave_mode) {
            std::string html = "<HTML>\r\n<HEAD>Directory list of '" + path + "'</HEAD>\r\n<BODY><UL>\r\n";
            Glib::Dir dir(full_path);
            std::string d;
            std::string p;
            if (path == "/") {
                p = "";
            } else {
                p = path;
            }
            while ((d = dir.read_name()) != "") {
                html += "<LI><a href=\""+ base_url + p + "/"+d+"\">"+d+"</a></LI>\r\n";
            }
            html += "</UL></BODY></HTML>";
            Arc::PayloadRaw *buf = new Arc::PayloadRaw();
            buf->Insert(html.c_str(), 0, html.length());
            return buf;
        }
    }
    return NULL;
}

static off_t GetEntitySize(Arc::MessagePayload &payload) {
    try {
        return dynamic_cast<Arc::PayloadRawInterface&>(payload).Size();
    } catch (std::exception &e) {
    }
    return 0;
}


Arc::MCC_Status Hopi::Put(const std::string &path, Arc::MessagePayload &payload)
{
    // XXX eliminate relative paths first
    logger.msg(Arc::VERBOSE, "PUT called");
    std::string full_path = Glib::build_filename(doc_root, path);
    if (slave_mode && (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == false)) {
        logger.msg(Arc::ERROR, "Hopi SlaveMode is active, PUT is only allowed to existing files");        
        return Arc::MCC_Status();
    }
    HopiFile fd(full_path.c_str(),false,slave_mode);
    if (!fd) {
        return Arc::MCC_Status();
    }
    fd.Size(GetEntitySize(payload));
    logger.msg(Arc::DEBUG, "File size is %u",fd.Size());
    try {
        Arc::PayloadStreamInterface& stream = dynamic_cast<Arc::PayloadStreamInterface&>(payload);
        char sbuf[1024*1024];
        for(;;) {
            int size = sizeof(sbuf);
            off_t offset = stream.Pos();
            if(!stream.Get(sbuf,size)) {
                if(!stream) {
                    logger.msg(Arc::VERBOSE, "error reading from HTTP stream");
                    return Arc::MCC_Status();
                }
                break;
            }
            if(fd.Write(sbuf,offset,size) != size) {
              logger.msg(Arc::VERBOSE, "error on write");
              return Arc::MCC_Status();
            }
        }
    } catch (std::exception &e) {
        try {
            Arc::PayloadRawInterface& buf = dynamic_cast<Arc::PayloadRawInterface&>(payload);
            for(int n = 0;;++n) {
                char* sbuf = buf.Buffer(n);
                if(sbuf == NULL) break;
                off_t offset = buf.BufferPos(n);
                size_t size = buf.BufferSize(n);
                if(size > 0) {
                    if(fd.Write(sbuf,offset,size) != size) {
                        logger.msg(Arc::VERBOSE, "error on write");
                        return Arc::MCC_Status();
                    }
                }
            }
        } catch (std::exception &e) {
            logger.msg(Arc::ERROR, "Input for PUT operation is neither stream nor buffer");
            return Arc::MCC_Status();
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

static std::string GetPath(Arc::Message &inmsg,std::string &base) {
  base = inmsg.Attributes()->get("HTTP:ENDPOINT");
  Arc::AttributeIterator iterator = inmsg.Attributes()->getAll("PLEXER:EXTENSION");
  std::string path;
  if(iterator.hasMore()) {
    // Service is behind plexer
    path = *iterator;
    if(base.length() > path.length()) base.resize(base.length()-path.length());
  } else {
    // Standalone service
    path=Arc::URL(base).Path();
    base.resize(0);
  };
  return path;
}

Arc::MCC_Status Hopi::process(Arc::Message &inmsg, Arc::Message &outmsg)
{
    std::string method = inmsg.Attributes()->get("HTTP:METHOD");
    std::string base_url;
    std::string path = GetPath(inmsg,base_url);

    logger.msg(Arc::VERBOSE, "method=%s, path=%s, url=%s, base=%s", method, path, inmsg.Attributes()->get("HTTP:ENDPOINT"), base_url);
    // Do file cleaning here to avoid running dedicated thread
    HopiFile::DestroyStuck();
    HopiFileTimeout::DestroyOld();
    if (method == "GET") {
        off_t range_start = 0;
        off_t range_end = (off_t)(-1);
        {
          std::string val;
          val=inmsg.Attributes()->get("HTTP:RANGESTART");
          if(!val.empty()) { // Negative ranges not supported
            if(!Arc::stringto<off_t>(val,range_start)) {
              range_start=0;
            } else {
              val=inmsg.Attributes()->get("HTTP:RANGEEND");
              if(!val.empty()) {
                if(!Arc::stringto<off_t>(val,range_end)) {
                  range_end=(off_t)(-1);
                } else {
                  range_end+=1;
                };
              };
            };
          };
        };
        Arc::MessagePayload *buf = Get(path, base_url, range_start, range_end);
        if (!buf) {
            // XXX: HTTP error
            return Arc::MCC_Status();
        }
        outmsg.Payload(buf);
        return Arc::MCC_Status(Arc::STATUS_OK);
    } else if (method == "PUT") {
        Arc::MessagePayload *inpayload = inmsg.Payload();
        if(!inpayload) {
            logger.msg(Arc::WARNING, "No content provided for PUT operation");
            return Arc::MCC_Status();
        }
        Arc::MCC_Status ret = Put(path, *inpayload);
        if (!ret) {
            // XXX: HTTP error
            return Arc::MCC_Status();
        }
        Arc::PayloadRaw *buf = new Arc::PayloadRaw();
        outmsg.Payload(buf);
        return ret;
    } 
    logger.msg(Arc::WARNING, "Not supported operation");
    return Arc::MCC_Status();
}

} // namespace Hopi

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "hopi", "HED:SERVICE", 0, &Hopi::get_service },
    { NULL, NULL, 0, NULL}
};
