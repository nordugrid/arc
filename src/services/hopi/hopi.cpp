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
  typedef std::list<std::pair<off_t,off_t> > chunks_t;
  chunks_t chunks;
  off_t size;
  time_t last_accessed;
  int refcount;
  std::map<std::string,HopiFileChunks>::iterator self;
  HopiFileChunks(void);
 public:
  void Add(off_t start,off_t end);
  off_t Size(void) { return size; };
  void Size(off_t size) {
    lock.lock();
    if(size > HopiFileChunks::size) HopiFileChunks::size = size;
    lock.unlock();
  };
  static HopiFileChunks& Get(std::string path);
  void Remove(void);
  void Release(void);
  bool Complete(void);
  void Print(void);
};

std::map<std::string,HopiFileChunks> HopiFileChunks::files;
Glib::Mutex HopiFileChunks::lock;

void HopiFileChunks::Print(void) {
  int n = 0;
  for(chunks_t::iterator c = chunks.begin();c!=chunks.end();++c) {
    Hopi::logger.msg(Arc::VERBOSE, "Chunk %u: %u - %u",n,c->first,c->second);
  };
}

HopiFileChunks::HopiFileChunks(void):size(0),last_accessed(time(NULL)),refcount(0),self(files.end()) {
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
  int Write(void* buf,off_t offset,int size);
  int Read(void* buf,off_t offset,int size);
  void Size(off_t size) { chunks.Size(size); };
  operator bool(void) { return (handle != -1); };
  bool operator!(void) { return (handle == -1); };
  void Destroy(void);
};

HopiFile::HopiFile(const std::string& path,bool for_read,bool slave):handle(-1),chunks(HopiFileChunks::Get(path)) {
  HopiFile::for_read=for_read;
  HopiFile::slave=slave;
  HopiFile::path=path;
  if(for_read) {
    handle=open(path.c_str(),O_RDONLY);
  } else {
    if(slave) {
      handle=open(path.c_str(),O_WRONLY);
      if(handle == -1) {
        if(errno == ENOENT) {
          Hopi::logger.msg(Arc::ERROR, "Hopi SlaveMode is active, PUT is only allowed to existing files");
        }
      }
    } else {
      handle=open(path.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
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
          Hopi::logger.msg(Arc::ERROR, "Removing complete file in slave mode");
          unlink(path.c_str());
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
  unlink(path.c_str());
  chunks.Remove();
}

int HopiFile::Write(void* buf,off_t offset,int size) {
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

int HopiFile::Read(void* buf,off_t offset,int size) {
  if(handle == -1) return -1;
  if(!for_read) return -1;
  if(lseek(handle,offset,SEEK_SET) != offset) return 0;
  return read(handle,buf,size);
}


Hopi::Hopi(Arc::Config *cfg):Service(cfg)
{
    logger.msg(Arc::INFO, "Hopi Initialized"); 
    doc_root = (std::string)((*cfg)["DocumentRoot"]);
    if (doc_root.empty()) {
        doc_root = "./";
    }
    logger.msg(Arc::INFO, "Hopi DocumentRoot is " + doc_root);
    slave_mode = (std::string)((*cfg)["SlaveMode"]);
    if (slave_mode.empty()) {
        slave_mode = "0";
    }  
    if (slave_mode == "1") logger.msg(Arc::INFO, "Hopi SlaveMode is on!");
}

Hopi::~Hopi(void)
{
    logger.msg(Arc::INFO, "Hopi shutdown");
}

Arc::PayloadRawInterface *Hopi::Get(const std::string &path, const std::string &base_url)
{
    // XXX eliminate relativ paths first
    std::string full_path = Glib::build_filename(doc_root, path);
    if (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == true) {
        if (Glib::file_test(full_path, Glib::FILE_TEST_IS_REGULAR) == true) {
            PayloadFile * pf = new PayloadFile(full_path.c_str());
            if (slave_mode == "1") unlink(full_path.c_str());
            return pf;
        } else if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR) && slave_mode != "1") {
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
    // XXX eliminate relativ paths first
    logger.msg(Arc::DEBUG, "PUT called");
    std::string full_path = Glib::build_filename(doc_root, path);
    if ((slave_mode == "1") && (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == false)) {
        logger.msg(Arc::ERROR, "Hopi SlaveMode is active, PUT is only allowed to existing files");        
        return Arc::MCC_Status();
    }
    HopiFile fd(full_path.c_str(),false,slave_mode == "1");
    if (!fd) {
        return Arc::MCC_Status();
    }
    fd.Size(GetEntitySize(payload));
    try {
        Arc::PayloadStreamInterface& stream = dynamic_cast<Arc::PayloadStreamInterface&>(payload);
        char sbuf[1024*1024];
        for(;;) {
            int size = sizeof(sbuf);
            off_t offset = stream.Pos();
            if(!stream.Get(sbuf,size)) {
                if(!stream) {
                    logger.msg(Arc::DEBUG, "error reading from HTTP stream");
                    return Arc::MCC_Status();
                }
                break;
            }
            if(fd.Write(sbuf,offset,size) != size) {
              logger.msg(Arc::DEBUG, "error on write");
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
                        logger.msg(Arc::DEBUG, "error on write");
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

    logger.msg(Arc::DEBUG, "method=%s, path=%s, url=%s, base=%s", method, path, inmsg.Attributes()->get("HTTP:ENDPOINT"), base_url);
    if (method == "GET") {
        Arc::PayloadRawInterface *buf = Get(path, base_url);
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
