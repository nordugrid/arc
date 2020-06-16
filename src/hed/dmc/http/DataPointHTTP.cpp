// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_LIMIT_MACROS
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <map>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/Utils.h>

#include "StreamBuffer.h"
#include "DataPointHTTP.h"

namespace ArcDMCHTTP {

using namespace Arc;

  // TODO: filling failure_code in multiple threads is potentailly dangerous
  //       because it is accessible through public method. Maybe it would be
  //       safer to fill it in StopReading/StopWriting

  Logger DataPointHTTP::logger(Logger::getRootLogger(), "DataPoint.HTTP");

  typedef struct {
    DataPointHTTP *point;
  } HTTPInfo_t;

  class ChunkControl {
  private:
    typedef struct {
      uint64_t start;
      uint64_t end;
    } chunk_t;
    std::list<chunk_t> chunks_;
    Glib::Mutex lock_;
  public:
    ChunkControl(uint64_t size = UINT64_MAX);
    ~ChunkControl();
    // Get chunk to be transferred. On input 'length'
    // contains maximal acceptable chunk size.
    bool Get(uint64_t& start, uint64_t& length);
    // Report chunk transferred. It may be _different_
    // from one obtained through Get().
    void Claim(uint64_t start, uint64_t length);
    void Claim(uint64_t length);
    // Report chunk not transferred. It must be
    // _same_ as one obtained by Get().
    void Unclaim(uint64_t start, uint64_t length);
  };

  class PayloadMemConst
    : public PayloadRawInterface {
  private:
    char *buffer_;
    uint64_t begin_;
    uint64_t end_;
    uint64_t size_;
  public:
    PayloadMemConst(void *buffer,
                    uint64_t offset,
                    unsigned int length,
                    uint64_t size = 0)
      : buffer_((char*)buffer),
        begin_(offset),
        end_(offset + length),
        size_(size) {}
    virtual ~PayloadMemConst() {}
    virtual char operator[](Size_t pos) const {
      if (!buffer_)
        return 0;
      if (pos < begin_)
        return 0;
      if (pos >= end_)
        return 0;
      return buffer_[pos - begin_];
    }
    virtual char* Content(Size_t pos = -1) {
      if (!buffer_)
        return NULL;
      if (pos < begin_)
        return NULL;
      if (pos >= end_)
        return NULL;
      return
        buffer_ + (pos - begin_);
    }
    virtual Size_t Size() const {
      return size_;
    }
    virtual char* Insert(Size_t /* pos */ = 0, Size_t /* size */ = 0) {
      return NULL;
    }
    virtual char* Insert(const char* /* s */,
                         Size_t /* pos */ = 0, Size_t /* size */ = 0) {
      return NULL;
    }
    virtual char* Buffer(unsigned int num) {
      if (num != 0)
        return NULL;
      return buffer_;
    }
    virtual Size_t BufferSize(unsigned int num) const {
      if (!buffer_)
        return 0;
      if (num != 0)
        return 0;
      return
        end_ - begin_;
    }
    virtual Size_t BufferPos(unsigned int num) const {
      if (!buffer_)
        return 0;
      if (num != 0)
        return 0;
      return begin_;
    }
    virtual bool Truncate(Size_t /* size */) {
      return false;
    }
  };

  ChunkControl::ChunkControl(uint64_t size) {
    chunk_t chunk = {
      0, size
    };
    chunks_.push_back(chunk);
  }

  ChunkControl::~ChunkControl() {}

  bool ChunkControl::Get(uint64_t& start, uint64_t& length) {
    if (length == 0) return false;
    lock_.lock();
    std::list<chunk_t>::iterator c = chunks_.begin();
    if (c == chunks_.end()) {
      lock_.unlock();
      return false;
    }
    start = c->start;
    uint64_t l = (c->end) - (c->start);
    if (l <= length) {
      length = l;
      chunks_.erase(c);
    } else {
      c->start += length;
    }
    lock_.unlock();
    return true;
  }

  void ChunkControl::Claim(uint64_t start, uint64_t length) {
    if (length == 0) return;
    uint64_t end = start + length;
    lock_.lock();
    for (std::list<chunk_t>::iterator c = chunks_.begin();
         c != chunks_.end();) {
      if (end <= c->start) break;
      if ((start <= c->start) && (end >= c->end)) {
        start = c->end;
        length = end - start;
        c = chunks_.erase(c);
        if (length > 0) continue;
        break;
      }
      if ((start > c->start) && (end < c->end)) {
        chunk_t chunk;
        chunk.start = c->start;
        chunk.end = start;
        c->start = end;
        chunks_.insert(c, chunk);
        break;
      }
      if ((start <= c->start) && (end < c->end) && (end > c->start)) {
        c->start = end;
        break;
      }
      if ((start > c->start) && (start < c->end) && (end >= c->end)) {
        uint64_t start_ = c->end;
        c->end = start;
        start = start_;
        length = end - start;
        ++c;
        if (length > 0) continue;
        break;
      }
      ++c;
    }
    lock_.unlock();
  }

  void ChunkControl::Claim(uint64_t start) {
    Claim(start, UINT64_MAX - start);
  }

  void ChunkControl::Unclaim(uint64_t start, uint64_t length) {
    if (length == 0) return;
    uint64_t end = start + length;
    lock_.lock();
    for (std::list<chunk_t>::iterator c = chunks_.begin();
         c != chunks_.end(); ++c) {
      if ((end >= c->start) && (end <= c->end)) {
        if (start < c->start) c->start = start;
        lock_.unlock();
        return;
      }
      if ((start <= c->end) && (start >= c->start)) {
        if (end > c->end) {
          c->end = end;
          std::list<chunk_t>::iterator c_ = c;
          ++c_;
          while (c_ != chunks_.end())
            if (c->end >= c_->start) {
              if (c_->end >= c->end) {
                c->end = c_->end;
                break;
              }
              c_ = chunks_.erase(c_);
            }
            else break;
        }
        lock_.unlock();
        return;
      }
      if ((start <= c->start) && (end >= c->end)) {
        c->start = start;
        if (end > c->end) {
          c->end = end;
          std::list<chunk_t>::iterator c_ = c;
          ++c_;
          while (c_ != chunks_.end())
            if (c->end >= c_->start) {
              if (c_->end >= c->end) {
                c->end = c_->end;
                break;
              }
              c_ = chunks_.erase(c_);
            }
            else break;
        }
        lock_.unlock();
        return;
      }
      if (end < c->start) {
        chunk_t chunk = {
          start, end
        };
        chunks_.insert(c, chunk);
        lock_.unlock();
        return;
      }
    }
    chunk_t chunk = {
      start, end
    };
    chunks_.push_back(chunk);
    lock_.unlock();
  }

  DataPointHTTP::DataPointHTTP(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false),
      chunks(NULL),
      transfers_tofinish(0) {}

  DataPointHTTP::~DataPointHTTP() {
    StopReading();
    StopWriting();
    if (chunks) delete chunks;
    for(std::multimap<std::string,ClientHTTP*>::iterator cl = clients.begin(); cl != clients.end(); ++cl) {
      delete cl->second;
    };
  }

  Plugin* DataPointHTTP::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "http" &&
        ((const URL &)(*dmcarg)).Protocol() != "https" &&
        ((const URL &)(*dmcarg)).Protocol() != "httpg" &&
        ((const URL &)(*dmcarg)).Protocol() != "dav" &&
        ((const URL &)(*dmcarg)).Protocol() != "davs")
      return NULL;
    return new DataPointHTTP(*dmcarg, *dmcarg, dmcarg);
  }

  static bool html2list(const char *html, const URL& base,
                        std::list<FileInfo>& files) {
    for (const char *pos = html;;) {
      // Looking for tag
      const char *tag_start = strchr(pos, '<');
      if (!tag_start) break;              // No more tags
      // Looking for end of tag
      const char *tag_end = strchr(tag_start + 1, '>');
      if (!tag_end) return false;         // Broken html?
      // 'A' tag?
      if (strncasecmp(tag_start, "<A ", 3) == 0) {
        // Lookig for HREF
        const char *href = strstr(tag_start + 3, "href=");
        if (!href) href = strstr(tag_start + 3, "HREF=");
        if (href && (href < tag_end)) {
          const char *url_start = href + 5;
          const char *url_end = NULL;
          if ((*url_start) == '"') {
            ++url_start;
            url_end = strchr(url_start, '"');
            if ((!url_end) || (url_end > tag_end)) url_end = NULL;
          }
          else if ((*url_start) == '\'') {
            ++url_start;
            url_end = strchr(url_start, '\'');
            if ((!url_end) || (url_end > tag_end)) url_end = NULL;
          }
          else {
            url_end = strchr(url_start, ' ');
            if ((!url_end) || (url_end > tag_end)) url_end = tag_end;
          }
          if (!url_end) return false; // Broken HTML
          std::string url(url_start, url_end - url_start);
          url = uri_unencode(url);
          if(url[0] == '/') {
            url = base.ConnectionURL()+url;
          }
          if (url.find("://") != std::string::npos) {
            URL u(url);
            std::string b = base.str();
            if (b[b.size() - 1] != '/') b += '/';
            if (u.str().substr(0, b.size()) == b) url = u.str().substr(b.size());
          }
          if (url[0] != '?' && url[0] != '/') {
            if (url.find('/') == url.size() - 1) {
              std::list<FileInfo>::iterator f = files.insert(files.end(), url);
              f->SetType(FileInfo::file_type_dir);
            }
            else if (url.find('/') == std::string::npos) {
              std::list<FileInfo>::iterator f = files.insert(files.end(), url);
              f->SetType(FileInfo::file_type_file);
            }
          }
        }
      }
      pos = tag_end + 1;
    }
    return true;
  }

  DataStatus DataPointHTTP::do_stat_http(URL& rurl, FileInfo& file) {
    for(int redirects_max = 10;redirects_max>=0;--redirects_max) {
      PayloadRawInterface *inbuf = NULL;
      HTTPClientInfo info;
      PayloadRaw request;
      std::string path = rurl.FullPathURIEncoded();
      info.lastModified = (time_t)(-1);
      AutoPointer<ClientHTTP> client(acquire_client(rurl));
      if (!client) return DataStatus::StatError;
      // Do HEAD to obtain some metadata
      MCC_Status r = client->process("HEAD", path, &request, &info, &inbuf);
      if (inbuf) { delete inbuf; inbuf = NULL; }
      if (!r) {
        // Because there is no reliable way to check if connection
        // is still alive at this place, we must try again
        client = acquire_new_client(rurl);
        if(client) r = client->process("HEAD", path, &request, &info, &inbuf);
        if (inbuf) { delete inbuf; inbuf = NULL; }
        if(!r) return DataStatus(DataStatus::StatError,r.getExplanation());
      }
      release_client(rurl,client.Release());
      if (info.code != 200) {
        if((info.code == 301) || // permanent redirection
           (info.code == 302) || // temporary redirection
           (info.code == 303) || // POST to GET redirection
           (info.code == 304)) { // redirection to cache
          // 305 - redirection to proxy - unhandled
          // Recreate connection now to new URL
          rurl = info.location;
          logger.msg(VERBOSE,"Redirecting to %s",info.location.str());
          continue;
        }
        return DataStatus(DataStatus::StatError, http2errno(info.code), info.reason);
      }
      // Fetch known metadata
      std::string type = info.type;
      std::string::size_type pos = type.find(';');
      if (pos != std::string::npos) type = type.substr(0, pos);

      // Treat every html as potential directory/set of links
      if(type == "text/html") {
        file.SetType(FileInfo::file_type_dir);
      } else {
        file.SetType(FileInfo::file_type_file);
      }
      if(info.size != (uint64_t)(-1)) {
        file.SetSize(info.size);
      }
      if(info.lastModified != (time_t)(-1)) {
        file.SetModified(info.lastModified);
      }
      // Not sure
      if(info.location) file.AddURL(info.location.str());
      return DataStatus::Success;
    }
    return DataStatus(DataStatus::StatError,"Too many redirects");
  }

 static unsigned int parse_http_status(const std::string& str) {
   // HTTP/1.1 200 OK
   std::vector<std::string> tokens;
   tokenize(str, tokens);
   if(tokens.size() < 2) return 0;
   unsigned int code;
   if(!stringto(tokens[1],code)) return 0;
   return code;
 }

  static bool parse_webdav_response(XMLNode response, FileInfo& file, std::string& url) {
    bool found = false;
    XMLNode href = response["href"];
    XMLNode propstat = response["propstat"];
    for(;(bool)propstat;++propstat) {
      // Multiple propstat for multiple results
      // Only those with 200 represent real results
      std::string status = (std::string)propstat["status"];
      if(parse_http_status(status) != 200) continue;
      XMLNode prop = propstat["prop"];
      if((bool)prop) {
        XMLNode creationdate = prop["creationdate"];
        XMLNode displayname = prop["displayname"];
        XMLNode getcontentlength = prop["getcontentlength"];
        XMLNode resourcetype = prop["resourcetype"];
        XMLNode getlastmodified = prop["getlastmodified"];
        // Fetch known metadata
        if((bool)resourcetype) {
          if((bool)resourcetype["collection"]) {
            file.SetType(FileInfo::file_type_dir);
          } else {
            file.SetType(FileInfo::file_type_file);
          }
        }
        uint64_t l = (uint64_t)(-1);
        if(stringto((std::string)getcontentlength,l)) {
          file.SetSize(l);
        }
        std::string t = (std::string)getlastmodified;
        if(t.empty()) t = (std::string)creationdate;
        if(!t.empty()) {
          Time tm(t);
          if(tm.GetTime() != Time::UNDEFINED) {
            file.SetModified(tm);
          }
        }
        found = true;
      }
    }
    if(found) {
      if((bool)href) url = (std::string)href;
    }
    return found;
  }

  DataStatus DataPointHTTP::do_stat_webdav(URL& rurl, FileInfo& file) {
    PayloadRaw request;
    {
      NS webdav_ns("d","DAV:");
      XMLNode propfind(webdav_ns,"d:propfind");
      XMLNode props = propfind.NewChild("d:prop");
      props.NewChild("d:creationdate");
      props.NewChild("d:displayname");
      props.NewChild("d:getcontentlength");
      props.NewChild("d:resourcetype");
      props.NewChild("d:getlastmodified");
      std::string s; propfind.GetDoc(s);
      request.Insert(s.c_str(),0,s.length());
    }
    std::multimap<std::string, std::string> propattr;
    propattr.insert(std::pair<std::string, std::string>("Depth","0"));
    for(int redirects_max = 10;redirects_max>=0;--redirects_max) {
      std::string path = rurl.FullPathURIEncoded();
      AutoPointer<ClientHTTP> client(acquire_client(rurl));
      if (!client) return DataStatus::StatError;
      PayloadRawInterface *inbuf = NULL;
      HTTPClientInfo info;
      info.lastModified = (time_t)(-1);
      MCC_Status r = client->process("PROPFIND", path, propattr, &request, &info, &inbuf);
      if (!r) {
        if (inbuf) { delete inbuf; inbuf = NULL; }
        // Because there is no reliable way to check if connection
        // is still alive at this place, we must try again
        client = acquire_new_client(rurl);
        if(client) r = client->process("PROPFIND", path, propattr, &request, &info, &inbuf);
        if(!r) {
          if (inbuf) { delete inbuf; inbuf = NULL; }
          return DataStatus(DataStatus::StatError,r.getExplanation());
        }
      }
      if ((info.code != 200) && (info.code != 207)) { // 207 for multistatus response
        if (inbuf) { delete inbuf; inbuf = NULL; }
        release_client(rurl,client.Release());
        if((info.code == 301) || // permanent redirection
           (info.code == 302) || // temporary redirection
           (info.code == 303) || // POST to GET redirection
           (info.code == 304)) { // redirection to cache
          // 305 - redirection to proxy - unhandled
          // Recreate connection now to new URL
          rurl = info.location;
          logger.msg(VERBOSE,"Redirecting to %s",info.location.str());
          continue;
        }
        //  Possibly following errors can be returned by server
        // if it does not implement webdav.
        // 405 - method not allowed
        // 501 - not implemented
        // 500 - internal error (for simplest servers)
        if((info.code == 405) || (info.code == 501) || (info.code == 500)) {
          // Indicating possible failure reason using POSIX error code ENOSYS
          return DataStatus(DataStatus::StatError, ENOSYS, info.reason);
        }
        return DataStatus(DataStatus::StatError, http2errno(info.code), info.reason);
      }
      if(inbuf) {
        XMLNode multistatus(ContentFromPayload(*inbuf));
        delete inbuf; inbuf = NULL;
        release_client(rurl,client.Release());
        if(multistatus.Name() == "multistatus") {
          XMLNode response = multistatus["response"];
          if((bool)response) {
            std::string url;
            if(parse_webdav_response(response,file,url)) {
              return DataStatus::Success;
            }
          }
        }
      } else {
        release_client(rurl,client.Release());
      }
      return DataStatus(DataStatus::StatError,"Can't process WebDAV response");
    }
    return DataStatus(DataStatus::StatError,"Too many redirects");
  }

  DataStatus DataPointHTTP::do_list_webdav(URL& rurl, std::list<FileInfo>& files, DataPointInfoType verb) {
    PayloadRaw request;
    {
      NS webdav_ns("d","DAV:");
      XMLNode propfind(webdav_ns,"d:propfind");
      XMLNode props = propfind.NewChild("d:prop");
      // TODO: verb
      props.NewChild("d:creationdate");
      props.NewChild("d:displayname");
      props.NewChild("d:getcontentlength");
      props.NewChild("d:resourcetype");
      props.NewChild("d:getlastmodified");
      std::string s; propfind.GetDoc(s);
      request.Insert(s.c_str(),0,s.length());
    }
    std::multimap<std::string, std::string> propattr;
    propattr.insert(std::pair<std::string, std::string>("Depth","1")); // for listing
    for(int redirects_max = 10;redirects_max>=0;--redirects_max) {
      std::string path = rurl.FullPathURIEncoded();
      AutoPointer<ClientHTTP> client(acquire_client(rurl));
      if (!client) return DataStatus::StatError;
      PayloadRawInterface *inbuf = NULL;
      HTTPClientInfo info;
      info.lastModified = (time_t)(-1);
      MCC_Status r = client->process("PROPFIND", path, propattr, &request, &info, &inbuf);
      if (!r) {
        if (inbuf) { delete inbuf; inbuf = NULL; }
        // Because there is no reliable way to check if connection
        // is still alive at this place, we must try again
        client = acquire_new_client(rurl);
        if(client) r = client->process("PROPFIND", path, propattr, &request, &info, &inbuf);
        if(!r) {
          if (inbuf) { delete inbuf; inbuf = NULL; }
          return DataStatus(DataStatus::StatError,r.getExplanation());
        }
      }
      if ((info.code != 200) && (info.code != 207)) { // 207 for multistatus response
        if (inbuf) { delete inbuf; inbuf = NULL; }
        release_client(rurl,client.Release());
        if((info.code == 301) || // permanent redirection
           (info.code == 302) || // temporary redirection
           (info.code == 303) || // POST to GET redirection
           (info.code == 304)) { // redirection to cache
          // 305 - redirection to proxy - unhandled
          // Recreate connection now to new URL
          rurl = info.location;
          logger.msg(VERBOSE,"Redirecting to %s",info.location.str());
          continue;
        }
        //  Possibly following errors can be returned by server
        // if it does not implement webdav.
        // 405 - method not allowed
        // 501 - not implemented
        // 500 - internal error (for simplest servers)
        if((info.code == 405) || (info.code == 501) || (info.code == 500)) {
          // Indicating possible failure reason using POSIX error code ENOSYS
          return DataStatus(DataStatus::StatError, ENOSYS, info.reason);
        }
        return DataStatus(DataStatus::StatError, http2errno(info.code), info.reason);
      }
      if(inbuf) {
        XMLNode multistatus(ContentFromPayload(*inbuf));
        delete inbuf; inbuf = NULL;
        release_client(rurl,client.Release());
        if(multistatus.Name() == "multistatus") {
          XMLNode response = multistatus["response"];
          for(;(bool)response;++response) {
            FileInfo file;
            std::string url;
            if(parse_webdav_response(response,file,url)) {
              // url = uri_unencode(url); ?
              if(url[0] == '/') {
                url = rurl.ConnectionURL()+url;
              }
              if (url.find("://") != std::string::npos) {
                URL u(url);
                std::string b = rurl.str();
                if (b[b.size() - 1] != '/') b += '/';
                if (u.str().substr(0, b.size()) == b) url = u.str().substr(b.size());
              }
              if(!url.empty()) { // skip requested object
                file.SetName(url);
                files.push_back(file);
              }
            }
          }
          return DataStatus::Success;
        }
      } else {
        release_client(rurl,client.Release());
      }
      return DataStatus(DataStatus::StatError,"Can't process WebDAV response");
    }
    return DataStatus(DataStatus::StatError,"Too many redirects");
  }

  DataStatus DataPointHTTP::Stat(FileInfo& file, DataPointInfoType verb) {
    // verb is not used
    URL curl = url;
    DataStatus r = do_stat_webdav(curl, file);
    if(!r) {
      if(r.GetErrno() != ENOSYS) return r;
      r = do_stat_http(curl, file);
      if (!r) return r;
    }
    std::string name = url.FullPath();
    std::string::size_type p = name.rfind('/');
    while(p != std::string::npos) {
      if(p != name.length()-1) {
        name = name.substr(p+1);
        break;
      }
      name.resize(p);
      p = name.rfind('/');
    }
    file.SetName(name);
    if(file.CheckSize()) {
      size = file.GetSize();
      logger.msg(VERBOSE, "Stat: obtained size %llu", size);
    }
    if(file.CheckModified()) {
      modified = file.GetModified();
      logger.msg(VERBOSE, "Stat: obtained modification time %s", modified.str());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    if (transfers_started.get() != 0) return DataStatus(DataStatus::ListError, EARCLOGIC, "Currently reading");
    URL curl = url;
    DataStatus r;
    bool webdav_supported = true;
    {
      FileInfo file;
      r = do_stat_webdav(curl, file);
      if(!r) {
        webdav_supported = false;
        if(r.GetErrno() == ENOSYS) {
          r = do_stat_http(curl, file);
        }
      }
      if(r) {
        if(file.CheckSize()) size = file.GetSize();
        if(file.CheckModified()) modified = file.GetModified();
        if(file.GetType() != FileInfo::file_type_dir) return DataStatus(DataStatus::ListError, ENOTDIR);
      }
    }

    if(webdav_supported) {
      r = do_list_webdav(curl, files, verb);
      return r;
    }

    // If server has no webdav try to read content and present it as list of links

    DataBuffer buffer;

    // Read content of file
    // TODO: Reuse already redirected URL stored in curl
    r = StartReading(buffer);
    if (!r) return DataStatus(DataStatus::ListError, r.GetErrno(), r.GetDesc());

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;
    unsigned long long int maxsize = (10*1024*1024); // 10MB seems reasonable limit

    while (buffer.for_write() || !buffer.eof_read()) {
      if (buffer.for_write(handle, length, offset, true)) {
        if(offset >= maxsize) { buffer.is_written(handle); break; };
        if((offset+length) > maxsize) length = maxsize-offset;
        if((offset+length) > result.size()) result.resize(offset+length,'\0');
        result.replace(offset,length,buffer[handle], length);
        buffer.is_written(handle);
      }
    }

    r = StopReading();
    if (!r) return DataStatus(DataStatus::ListError, r.GetErrno(), r.GetDesc());

    // Convert obtained HTML into set of links
    bool is_html = false;
    bool is_body = false;
    std::string::size_type tagstart = 0;
    std::string::size_type tagend = 0;
    std::string::size_type titlestart = std::string::npos;
    std::string::size_type titleend = std::string::npos;
    do {
      tagstart = result.find('<', tagend);
      if (tagstart == std::string::npos) break;
      tagend = result.find('>', tagstart);
      if (tagend == std::string::npos) break;
      std::string tag = result.substr(tagstart + 1, tagend - tagstart - 1);
      std::string::size_type tag_e = tag.find(' ');
      if (tag_e != std::string::npos) tag.resize(tag_e);
      if (strcasecmp(tag.c_str(), "title") == 0) titlestart = tagend + 1;
      else if (strcasecmp(tag.c_str(), "/title") == 0) titleend = tagstart - 1;
      else if (strcasecmp(tag.c_str(), "html") == 0) is_html = true;
      else if (strcasecmp(tag.c_str(), "body") == 0) is_body = is_html;
    } while (!is_body);

    std::string title;
    if (titlestart != std::string::npos && titleend != std::string::npos) {
      title = result.substr(titlestart, titleend - titlestart + 1);
    }

    if (is_body) {
      // If it was redirected then links must be relative to new location. Or not?
      html2list(result.c_str(), curl, files);
      if(verb & (INFO_TYPE_TYPE | INFO_TYPE_TIMES | INFO_TYPE_CONTENT)) {
        for(std::list<FileInfo>::iterator f = files.begin(); f != files.end(); ++f) {
          URL furl(curl.str()+'/'+(f->GetName()));
          do_stat_http(furl, *f);
        }
      }
    }
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartReading(DataBuffer& buffer) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    if (transfers_started.get() != 0) return DataStatus(DataStatus::IsReadingError, EARCLOGIC);
    reading = true;
    int transfer_streams = 1;
    strtoint(url.Option("threads"),transfer_streams);
    if (transfer_streams < 1) transfer_streams = 1;
    if (transfer_streams > MAX_PARALLEL_STREAMS) transfer_streams = MAX_PARALLEL_STREAMS;
    DataPointHTTP::buffer = &buffer;
    if (chunks) delete chunks;
    chunks = new ChunkControl;
    transfer_lock.lock();
    transfers_tofinish = 0;
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      if (!CreateThreadFunction(&read_thread, info, &transfers_started)) {
        delete info;
      } else {
        ++transfers_tofinish;
      }
    }
    if (transfers_tofinish == 0) {
      transfer_lock.unlock();
      StopReading();
      return DataStatus::ReadStartError;
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;
    if (!buffer) return DataStatus(DataStatus::ReadStopError, EARCLOGIC, "Not reading");
    if(!buffer->eof_read()) buffer->error_read(true);
    while (transfers_started.get()) {
      transfers_started.wait(10000); // Just in case
    }
    if (chunks) delete chunks;
    chunks = NULL;
    transfers_tofinish = 0;
    if (buffer->error_read()) {
      buffer = NULL;
      return DataStatus::ReadError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartWriting(DataBuffer& buffer,
                                         DataCallback*) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    if (transfers_started.get() != 0) return DataStatus(DataStatus::IsWritingError, EARCLOGIC);
    writing = true;
    int transfer_streams = 1;
    strtoint(url.Option("threads"),transfer_streams);
    if (transfer_streams < 1) transfer_streams = 1;
    if (transfer_streams > MAX_PARALLEL_STREAMS) transfer_streams = MAX_PARALLEL_STREAMS;
    DataPointHTTP::buffer = &buffer;
    if (chunks) delete chunks;
    chunks = new ChunkControl;
    transfer_lock.lock();
    transfers_tofinish = 0;
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      if (!CreateThreadFunction(&write_thread, info, &transfers_started)) {
        delete info;
      } else {
        ++transfers_tofinish;
      }
    }
    if (transfers_tofinish == 0) {
      transfer_lock.unlock();
      StopWriting();
      return DataStatus::WriteStartError;
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopWriting() {
    if (!writing) return DataStatus::WriteStopError;
    writing = false;
    if (!buffer) return DataStatus(DataStatus::WriteStopError, EARCLOGIC, "Not writing");
    if(!buffer->eof_write()) buffer->error_write(true);
    while (transfers_started.get()) {
      transfers_started.wait(); // Just in case
    }
    if (chunks) {
      delete chunks;
    }
    chunks = NULL;
    transfers_tofinish = 0;
    if (buffer->error_write()) {
      buffer = NULL;
      return DataStatus::WriteError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Check(bool check_meta) {
    PayloadRaw request;
    PayloadRawInterface *inbuf = NULL;
    HTTPClientInfo info;
    AutoPointer<ClientHTTP> client(acquire_client(url));
    if (!client) return DataStatus::CheckError;
    MCC_Status r = client->process("GET", url.FullPathURIEncoded(), 0, 15,
                                  &request, &info, &inbuf);
    PayloadRawInterface::Size_t logsize = 0;
    if (inbuf){
      logsize = inbuf->Size();
      delete inbuf; inbuf = NULL;
    }
    if (!r) {
      client = acquire_new_client(url);
      if(client) r = client->process("GET", url.FullPathURIEncoded(), 0, 15,
                                    &request, &info, &inbuf);
      if (inbuf){
        logsize = inbuf->Size();
        delete inbuf; inbuf = NULL;
      }
      if(!r) return DataStatus(DataStatus::CheckError,r.getExplanation());
    }
    release_client(url,client.Release());
    if ((info.code != 200) && (info.code != 206)) return DataStatus(DataStatus::CheckError, http2errno(info.code), info.reason);
    size = logsize;
    logger.msg(VERBOSE, "Check: obtained size %llu", size);
    modified = info.lastModified;
    logger.msg(VERBOSE, "Check: obtained modification time %s", modified.str());
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Remove() {
    AutoPointer<ClientHTTP> client(acquire_client(url));
    PayloadRaw request;
    PayloadRawInterface *inbuf = NULL;
    HTTPClientInfo info;
    MCC_Status r = client->process("DELETE", url.FullPathURIEncoded(),
                                  &request, &info, &inbuf);
    if (inbuf) { delete inbuf; inbuf = NULL; }
    if(!r) {
      client = acquire_new_client(url);
      if(client) r = client->process("DELETE", url.FullPathURIEncoded(),
                                    &request, &info, &inbuf);
      if (inbuf) { delete inbuf; inbuf = NULL; }
      if(!r) return DataStatus(DataStatus::DeleteError,r.getExplanation());
    }
    release_client(url,client.Release());
    if ((info.code != 200) && (info.code != 202) && (info.code != 204)) {
      return DataStatus(DataStatus::DeleteError, http2errno(info.code), info.reason);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Rename(const URL& destination) {
    AutoPointer<ClientHTTP> client(acquire_client(url));
    PayloadRaw request;
    PayloadRawInterface *inbuf = NULL;
    HTTPClientInfo info;
    std::multimap<std::string, std::string> attributes;
    attributes.insert(std::pair<std::string, std::string>("Destination", url.ConnectionURL() + destination.FullPathURIEncoded()));
    MCC_Status r = client->process("MOVE", url.FullPathURIEncoded(),
                                   attributes, &request, &info, &inbuf);
    if (inbuf) { delete inbuf; inbuf = NULL; }
    if(!r) {
      client = acquire_new_client(url);
      if(client) r = client->process("MOVE", url.FullPathURIEncoded(),
                                     attributes, &request, &info, &inbuf);
      if (inbuf) { delete inbuf; inbuf = NULL; }
      if(!r) return DataStatus(DataStatus::RenameError,r.getExplanation());
    }
    release_client(url,client.Release());
    if ((info.code != 201) && (info.code != 204)) { 
      return DataStatus(DataStatus::RenameError, http2errno(info.code), info.reason);
    }
    return DataStatus::Success;
  }

  bool DataPointHTTP::read_single(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    URL client_url = point.url;
    AutoPointer<ClientHTTP> client(point.acquire_client(client_url));
    bool transfer_failure = false;
    int retries = 0;
    std::string path = point.CurrentLocation().FullPathURIEncoded();
    DataStatus failure_code;
    if (!client) return false;
    for(;;) {  // for retries
      HTTPClientInfo transfer_info;
      PayloadRaw request;
      PayloadStreamInterface *instream = NULL;
      MCC_Status r = client->process(ClientHTTPAttributes("GET", path),
                                     &request, &transfer_info,
                                     &instream);
      if (!r) {
        if (instream) delete instream;
        // Failed to transfer - retry.
        // 10 times in a row seems to be reasonable number
        // To make it sure all retriable errors are eliminated
        // connection is also re-established
        client = NULL;
        // TODO: mark failure?
        if ((++retries) > 10) {
          transfer_failure = true;
          failure_code = DataStatus(DataStatus::ReadError, r.getExplanation());
          break;
        }
        // Recreate connection
        client = point.acquire_new_client(client_url);
        if(client) continue;
        transfer_failure = true;
        break;
      }
      if((transfer_info.code == 301) || // permanent redirection
         (transfer_info.code == 302) || // temporary redirection
         (transfer_info.code == 303) || // POST to GET redirection
         (transfer_info.code == 304)) { // redirection to cache
        // 305 - redirection to proxy - unhandled
        if (instream) delete instream;
        // Recreate connection now to new URL
        point.release_client(client_url,client.Release()); // return client to poll
        client_url = transfer_info.location;
        logger.msg(VERBOSE,"Redirecting to %s",transfer_info.location.str());
        client = point.acquire_client(client_url);
        if (client) {
          path = client_url.FullPathURIEncoded();
          continue;
        }
        transfer_failure = true;
        break;
      }
      if ((transfer_info.code != 200) &&
          (transfer_info.code != 206)) { // HTTP error - retry?
        if (instream) delete instream;
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504)) {
          if ((++retries) <= 10) continue;
        }
        logger.msg(VERBOSE,"HTTP failure %u - %s",transfer_info.code,transfer_info.reason);
        std::string reason = Arc::tostring(transfer_info.code) + " - " + transfer_info.reason;
        failure_code = DataStatus(DataStatus::ReadError, point.http2errno(transfer_info.code), reason);
        transfer_failure = true;
        break;
      }
      if(!instream) {
        transfer_failure = true;
        break;
      }
      // pick up useful information from HTTP header
      point.modified = transfer_info.lastModified;
      retries = 0;
      // Pull from stream and store in buffer
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      uint64_t transfer_pos = 0;
      for(;;) {
        if (transfer_handle == -1) {
          if (!point.buffer->for_read(transfer_handle, transfer_size, true)) {
            // No transfer buffer - must be failure or close initiated
            // externally
            break;
          }
        }
        int l = transfer_size;
        uint64_t pos = instream->Pos();
        if(!instream->Get((*point.buffer)[transfer_handle],l)) {
          //  Trying to find out if stream ended due to error
          if(transfer_pos < instream->Size()) transfer_failure = true;
          break;
        }
        point.buffer->is_read(transfer_handle, l, pos);
        transfer_handle = -1;
        transfer_pos = pos + l;
      }
      if (transfer_handle != -1) point.buffer->is_read(transfer_handle, 0, 0);
      if (instream) delete instream;
      // End of transfer - either success or not retrying transfer of whole body
      break;
    }
    if (transfer_failure) {
      point.failure_code = failure_code;
    }
    point.release_client(client_url,client.Release());
    return !transfer_failure;
  }

  void DataPointHTTP::read_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    point.transfer_lock.lock();
    point.transfer_lock.unlock();
    URL client_url = point.url;
    AutoPointer<ClientHTTP> client(point.acquire_client(client_url));
    bool transfer_failure = false;
    int retries = 0;
    std::string path = point.CurrentLocation().FullPathURIEncoded();
    DataStatus failure_code;
    bool partial_read_allowed = (client_url.Option("httpgetpartial") == "yes");
    if(partial_read_allowed) for (;;) {
      if(client && client->GetClosed()) client = point.acquire_client(client_url);
      if (!client) {
        transfer_failure = true;
        break;
      }
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      // get first buffer
      if (!point.buffer->for_read(transfer_handle, transfer_size, true)) {
        // No transfer buffer - must be failure or close initiated externally
        break;
      }
      uint64_t transfer_offset = 0;
      uint64_t chunk_length = 1024*1024;
      if(transfer_size > chunk_length) chunk_length = transfer_size;
      if (!(point.chunks->Get(transfer_offset, chunk_length))) {
        // No more chunks to transfer - quit this thread.
        point.buffer->is_read(transfer_handle, 0, 0);
        break;
      }
      uint64_t transfer_end = transfer_offset + chunk_length - 1;
      // Read chunk
      HTTPClientInfo transfer_info;
      PayloadRaw request;
      PayloadRawInterface *inbuf = NULL;
      MCC_Status r = client->process("GET", path, transfer_offset,
                                     transfer_end, &request, &transfer_info,
                                     &inbuf);
      if (!r) {
        // Return buffer
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf) delete inbuf;
        client = NULL;
        // Failed to transfer chunk - retry.
        // 10 times in a row seems to be reasonable number
        // TODO: mark failure?
        if ((++retries) > 10) {
          transfer_failure = true;
          failure_code = DataStatus(DataStatus::ReadError, r.getExplanation());
          break;
        }
        // Recreate connection
        client = point.acquire_new_client(client_url);
        if(client) continue;
        transfer_failure = true;
        break;
      }
      if (transfer_info.code == 416) { // EOF
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf) delete inbuf;
        break;
      }
      if((transfer_info.code == 301) || // permanent redirection
         (transfer_info.code == 302) || // temporary redirection
         (transfer_info.code == 303) || // POST to GET redirection
         (transfer_info.code == 304)) { // redirection to cache
        // 305 - redirection to proxy - unhandled
        // Return buffer
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf) delete inbuf;
        // Recreate connection now to new URL
        point.release_client(client_url,client.Release());
        client_url = transfer_info.location;
        logger.msg(VERBOSE,"Redirecting to %s",transfer_info.location.str());
        client = point.acquire_client(client_url);
        if (client) {
          path = client_url.FullPathURIEncoded();
          continue;
        }
        transfer_failure = true;
        break;
      }
      if ((transfer_info.code != 200) &&
          (transfer_info.code != 206)) { // HTTP error - retry?
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf) delete inbuf;
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504)) {
          // Retriable error codes
          if ((++retries) <= 10) continue;
        }
        logger.msg(VERBOSE,"HTTP failure %u - %s",transfer_info.code,transfer_info.reason);
        std::string reason = Arc::tostring(transfer_info.code) + " - " + transfer_info.reason;
        failure_code = DataStatus(DataStatus::ReadError, point.http2errno(transfer_info.code), reason);
        transfer_failure = true;
        break;
      }
      PayloadStreamInterface* instream = NULL;
      try {
        instream = dynamic_cast<PayloadStreamInterface*>(dynamic_cast<MessagePayload*>(inbuf));
      } catch(std::exception& e) {
        transfer_failure = true;
        break;
      }
      if(!instream) {
        transfer_failure = true;
        break;
      }
      // pick up useful information from HTTP header
      point.modified = transfer_info.lastModified;
      retries = 0;
      // Exclude chunks after EOF. Normally that is not needed.
      // But Apache if asked about out of file range gets confused
      // and sends *whole* file instead of 416.
      if(inbuf && (inbuf->Size() > 0)) point.chunks->Claim(inbuf->Size());
      bool whole = (inbuf && (((transfer_info.size == inbuf->Size() &&
                               (inbuf->BufferPos(0) == 0))) ||
                    inbuf->Size() == -1));
      point.transfer_lock.lock();
      point.chunks->Unclaim(transfer_offset, chunk_length);
      uint64_t transfer_pos = 0;
      for(;;) {
        if (transfer_handle == -1) {
          point.transfer_lock.unlock();
          if (!point.buffer->for_read(transfer_handle, transfer_size, true)) {
            // No transfer buffer - must be failure or close initiated
            // externally
            point.transfer_lock.lock();
            break;
          }
          point.transfer_lock.lock();
        }
        int l = transfer_size;
        uint64_t pos = instream->Pos();
        if(!instream->Get((*point.buffer)[transfer_handle],l)) {
          break;
        }
        point.buffer->is_read(transfer_handle, l, pos);
        point.chunks->Claim(pos, l);
        transfer_handle = -1;
        transfer_pos = pos + l;
      }
      if (transfer_handle != -1) point.buffer->is_read(transfer_handle, 0, 0);
      if (inbuf) delete inbuf;
      // If server returned chunk which is not overlaping requested one - seems
      // like server has nothing to say any more.
      if (transfer_pos <= transfer_offset) whole = true;
      point.transfer_lock.unlock();
      if (whole) break;
    }
    point.transfer_lock.lock();
    --(point.transfers_tofinish);
    if (transfer_failure) {
      point.failure_code = failure_code;
      point.buffer->error_read(true);
    }
    if (point.transfers_tofinish == 0) {
      // TODO: process/report failure?
      if(!partial_read_allowed) {
        // Reading in single chunk to be done in single thread
        if(!read_single(arg)) {
          transfer_failure = true;
          point.buffer->error_read(true);
        }
      }
      point.buffer->eof_read(true);
    }
    point.release_client(client_url,client.Release());
    delete &info;
    point.transfer_lock.unlock();
  }

  bool DataPointHTTP::write_single(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    URL client_url = point.url;
    AutoPointer<ClientHTTP> client(point.acquire_client(client_url));
    if (!client) return false;
    std::string path = client_url.FullPathURIEncoded();
    // TODO: Do ping to *client in order to check if connection is alive.
    // TODO: But ping itself can destroy connection on 1.0-like servers.
    // TODO: Hence retry is needed like in other cases.

    // To allow for redirection from the server without uploading the whole
    // body we send request header with Expect: 100-continue. Servers
    // should return either 100 continue or 30x redirection without 
    // waiting for body.
    bool expect100 = true;
    for (;;) {
      std::multimap<std::string, std::string> attrs;
      if(expect100) {
        attrs.insert(std::pair<std::string, std::string>("EXPECT", "100-continue"));
        // Note: there will be no 100 in response because it will be processed
        // at lower level.
      }
      StreamBuffer request(*point.buffer);
      if (point.CheckSize()) request.Size(point.GetSize());
      PayloadRawInterface *response = NULL;
      HTTPClientInfo transfer_info;
      MCC_Status r = client->process(ClientHTTPAttributes("PUT", path, attrs),
                                     &request, &transfer_info, &response);
      if (response) { delete response; response = NULL; }
      if (!r) {
        // It is not clear how to retry if early chunks are not available anymore.
        // Let it retry at higher level.
        point.failure_code = DataStatus(DataStatus::WriteError, r.getExplanation());
        client = NULL;
        return false;
      }
      if (transfer_info.code == 301 || // Moved permanently
          transfer_info.code == 302 || // Found (temp redirection)
          transfer_info.code == 307) { // Temporary redirection
        // Got redirection response
        // Recreate connection now to new URL
        point.release_client(client_url,client.Release());
        client_url = transfer_info.location;
        logger.msg(VERBOSE,"Redirecting to %s",transfer_info.location.str());
        client = point.acquire_client(client_url);
        if (client) {
          // TODO: Only one redirection is currently supported. Here we should
          // try again with 100-continue but there were problems with dCache
          // where on upload of the real body the server returns 201 Created
          // immediately and leaves an empty file. We cannot use a new
          // connection after 100 continue because redirected URLs that dCache
          // sends are one time use only.
          // TODO: make it configurable. Maybe through redirection depth.
          expect100 = false;
          path = client_url.FullPathURIEncoded();
          continue;
        }
        // Failed to acquire client for new connection - fail.
        point.buffer->error_write(true);
        point.failure_code = DataStatus(DataStatus::WriteError, "Failed to connect to redirected URL "+client_url.fullstr());
        return false;
      }
      if (transfer_info.code == 417) { // Expectation not supported
        // Retry without expect: 100 - probably old server
        expect100 = false;
        continue;
      }
      // RFC2616 says "Many older HTTP/1.0 and HTTP/1.1 applications do not
      // understand the Expect header". But this is not currently treated very well.
      if ((transfer_info.code != 201) &&
          (transfer_info.code != 200) &&
          (transfer_info.code != 204)) {  // HTTP error
        point.release_client(client_url,client.Release());
        point.failure_code = DataStatus(DataStatus::WriteError, point.http2errno(transfer_info.code), transfer_info.reason);
        return false;
      }
      // Looks like request passed well
      break;
    }
    point.release_client(client_url,client.Release());
    return true;
  }

  void DataPointHTTP::write_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    point.transfer_lock.lock();
    point.transfer_lock.unlock();
    URL client_url = point.url;
    AutoPointer<ClientHTTP> client(point.acquire_client(client_url));
    bool transfer_failure = false;
    int retries = 0;
    std::string path = client_url.FullPathURIEncoded();
    bool partial_write_failure = (client_url.Option("httpputpartial") != "yes");
    DataStatus failure_code;
    // Fall through if partial PUT is not allowed
    if(!partial_write_failure) for (;;) {
      if(client && client->GetClosed()) client = point.acquire_client(client_url);
      if (!client) {
        transfer_failure = true;
        break;
      }
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      unsigned long long int transfer_offset = 0;
      // get first buffer
      if (!point.buffer->for_write(transfer_handle, transfer_size,
                                   transfer_offset, true))
        // No transfer buffer - must be failure or close initiated externally
        break;
      //uint64_t transfer_offset = 0;
      //uint64_t transfer_end = transfer_offset+transfer_size;
      // Write chunk
      HTTPClientInfo transfer_info;
      PayloadMemConst request((*point.buffer)[transfer_handle],
                              transfer_offset, transfer_size,
                              point.CheckSize() ? point.GetSize() : 0);
      PayloadRawInterface *response = NULL;
      MCC_Status r = client->process("PUT", path, &request, &transfer_info,
                                     &response);
      if (response) delete response;
      response = NULL;
      if (!r) {
        client = NULL;
        // Failed to transfer chunk - retry.
        // 10 times in a row seems to be reasonable number
        // TODO: mark failure?
        if ((++retries) > 10) {
          transfer_failure = true;
          failure_code = DataStatus(DataStatus::WriteError, r.getExplanation());
          break;
        }
        // Return buffer
        point.buffer->is_notwritten(transfer_handle);
        // Recreate connection
        client = point.acquire_new_client(client_url);
        continue;
      }
      if ((transfer_info.code != 201) &&
          (transfer_info.code != 200) &&
          (transfer_info.code != 204)) {  // HTTP error - retry?
        point.buffer->is_notwritten(transfer_handle);

        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504)) {
          if ((++retries) <= 10) continue;
        }
        if (transfer_info.code == 501) { 
          // Not implemented - probably means server does not accept patial PUT
          partial_write_failure = true;
        } else {
          transfer_failure = true;
          failure_code = DataStatus(DataStatus::WriteError, point.http2errno(transfer_info.code), transfer_info.reason);
        }
        break;
      }
      retries = 0;
      point.buffer->is_written(transfer_handle);
    }
    point.transfer_lock.lock();
    --(point.transfers_tofinish);
    if (transfer_failure) {
      point.failure_code = failure_code;
      point.buffer->error_write(true);
    }
    if (point.transfers_tofinish == 0) {
      if(partial_write_failure) {
        // Writing in single chunk to be done in single thread
        if(!write_single(arg)) {
          transfer_failure = true;
          point.buffer->error_write(true);
        }
      }
      // TODO: process/report failure?
      point.buffer->eof_write(true);
      if ((!partial_write_failure) && (!(point.buffer->error())) && (point.buffer->eof_position() == 0)) {
        // Zero size data was transferred - must send at least one empty packet
        for (;;) {
          if (!client) client = point.acquire_client(client_url);
          if (!client) {
            point.buffer->error_write(true);
            break;
          }
          HTTPClientInfo transfer_info;
          PayloadMemConst request(NULL, 0, 0, 0);
          PayloadRawInterface *response = NULL;
          MCC_Status r = client->process("PUT", path, &request, &transfer_info,
                                         &response);
          if (response) delete response;
          response = NULL;
          if (!r) {
            client = NULL;
            if ((++retries) > 10) {
              point.failure_code = DataStatus(DataStatus::WriteError, r.getExplanation());
              point.buffer->error_write(true);
              break;
            }
            // Recreate connection
            client = point.acquire_new_client(client_url);;
            continue;
          }
          if ((transfer_info.code != 201) &&
              (transfer_info.code != 200) &&
              (transfer_info.code != 204)) {  // HTTP error - retry?
            if ((transfer_info.code == 500) ||
                (transfer_info.code == 503) ||
                (transfer_info.code == 504))
              if ((++retries) <= 10) continue;
            point.buffer->error_write(true);
            point.failure_code = DataStatus(DataStatus::WriteError, point.http2errno(transfer_info.code), transfer_info.reason);
            break;
          }
          break;
        }
      }
    }
    point.release_client(client_url,client.Release());
    delete &info;
    point.transfer_lock.unlock();
  }

  bool DataPointHTTP::SetURL(const URL& url) {
    if(url.Protocol() != this->url.Protocol()) return false;
    if(url.Host() != this->url.Host()) return false;
    if(url.Port() != this->url.Port()) return false;
    this->url = url;
    if(triesleft < 1) triesleft = 1;
    ResetMeta();
    return true;
  }

  ClientHTTP* DataPointHTTP::acquire_client(const URL& curl) {
    // TODO: lock
    if(!curl) return NULL;
    if((curl.Protocol() != "http") &&
       (curl.Protocol() != "https") &&
       (curl.Protocol() != "httpg") &&
       (curl.Protocol() != "dav") &&
       (curl.Protocol() != "davs")) return NULL;
    ClientHTTP* client = NULL;
    std::string key = curl.ConnectionURL();
    clients_lock.lock();
    std::multimap<std::string,ClientHTTP*>::iterator cl = clients.find(key);
    if(cl != clients.end()) {
      client = cl->second;
      clients.erase(cl);
      clients_lock.unlock();
    } else {
      clients_lock.unlock();
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      client = new ClientHTTP(cfg, curl, usercfg.Timeout());
    };
    return client;
  }

  ClientHTTP* DataPointHTTP::acquire_new_client(const URL& curl) {
    if(!curl) return NULL;
    if((curl.Protocol() != "http") &&
       (curl.Protocol() != "https") &&
       (curl.Protocol() != "httpg") &&
       (curl.Protocol() != "dav") &&
       (curl.Protocol() != "davs")) return NULL;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    return new ClientHTTP(cfg, curl, usercfg.Timeout());
  }

  void DataPointHTTP::release_client(const URL& curl, ClientHTTP* client) {
    if(!client) return;
    if(client->GetClosed()) { delete client; return; }
    std::string key = curl.ConnectionURL();
    //if(!*client) return;
    clients_lock.lock();
    clients.insert(std::pair<std::string,ClientHTTP*>(key,client));
    clients_lock.unlock();
  }

  int DataPointHTTP::http2errno(int http_code) const {
    // Codes taken from RFC 2616 section 10. Only 4xx and 5xx are treated as errors
    switch(http_code) {
      case 400:
      case 405:
      case 411:
      case 413:
      case 414:
      case 415:
      case 416:
      case 417:
        return EINVAL;
      case 401:
      case 403:
      case 407:
        return EACCES;
      case 404:
      case 410:
        return ENOENT;
      case 406:
      case 412:
        return EARCRESINVAL;
      case 408:
        return ETIMEDOUT;
      case 409: // Conflict. Not sure about this one.
      case 500:
      case 502:
      case 503:
      case 504:
        return EARCSVCTMP;
      case 501:
      case 505:
        return EOPNOTSUPP;

      default:
        return EARCOTHER;
    }
  }

} // namespace Arc

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "http", "HED:DMC", "HTTP, HTTP over SSL (https) or DAV(s)", 0, &ArcDMCHTTP::DataPointHTTP::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
