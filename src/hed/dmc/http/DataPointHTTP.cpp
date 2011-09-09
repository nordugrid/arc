// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_LIMIT_MACROS
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/client/ClientInterface.h>

#include "DataPointHTTP.h"

namespace Arc {

  Logger DataPointHTTP::logger(Logger::getRootLogger(), "DataPoint.HTTP");

  typedef struct {
    DataPointHTTP *point;
    ClientHTTP *client;
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

  ChunkControl::ChunkControl(uint64_t /* size */) {
    chunk_t chunk = {
      0, UINT64_MAX
    };
    chunks_.push_back(chunk);
  }

  ChunkControl::~ChunkControl() {}

  bool ChunkControl::Get(uint64_t& start, uint64_t& length) {
    if (length == 0)
      return false;
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
    }
    else
      c->start += length;
    lock_.unlock();
    return true;
  }

  void ChunkControl::Claim(uint64_t start, uint64_t length) {
    if (length == 0)
      return;
    uint64_t end = start + length;
    lock_.lock();
    for (std::list<chunk_t>::iterator c = chunks_.begin();
         c != chunks_.end();) {
      if (end <= c->start)
        break;
      if ((start <= c->start) && (end >= c->end)) {
        start = c->end;
        length = end - start;
        c = chunks_.erase(c);
        if (length > 0)
          continue;
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
        if (length > 0)
          continue;
        break;
      }
      ++c;
    }
    lock_.unlock();
  }

  void ChunkControl::Unclaim(uint64_t start, uint64_t length) {
    if (length == 0)
      return;
    uint64_t end = start + length;
    lock_.lock();
    for (std::list<chunk_t>::iterator c = chunks_.begin();
         c != chunks_.end(); ++c) {
      if ((end >= c->start) && (end <= c->end)) {
        if (start < c->start)
          c->start = start;
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
            else
              break;
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
            else
              break;
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
    lock_.unlock();
  }

  DataPointHTTP::DataPointHTTP(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      chunks(NULL),
      transfers_tofinish(0) {
    valid_url_options.push_back("tcpnodelay");
  }

  DataPointHTTP::~DataPointHTTP() {
    StopReading();
    StopWriting();
    if (chunks)
      delete chunks;
  }

  Plugin* DataPointHTTP::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "http" &&
        ((const URL &)(*dmcarg)).Protocol() != "https" &&
        ((const URL &)(*dmcarg)).Protocol() != "httpg")
      return NULL;
    return new DataPointHTTP(*dmcarg, *dmcarg);
  }

  static bool html2list(const char *html, const URL& base,
                        std::list<FileInfo>& files) {
    for (const char *pos = html;;) {
      // Looking for tag
      const char *tag_start = strchr(pos, '<');
      if (!tag_start)
        break;              // No more tags
      // Looking for end of tag
      const char *tag_end = strchr(tag_start + 1, '>');
      if (!tag_end)
        return false;            // Broken html?
      // 'A' tag?
      if (strncasecmp(tag_start, "<A ", 3) == 0) {
        // Lookig for HREF
        const char *href = strstr(tag_start + 3, "href=");
        if (!href)
          href = strstr(tag_start + 3, "HREF=");
        if (href) {
          const char *url_start = href + 5;
          const char *url_end = NULL;
          if ((*url_start) == '"') {
            ++url_start;
            url_end = strchr(url_start, '"');
            if ((!url_end) || (url_end > tag_end))
              url_end = NULL;
          }
          else if ((*url_start) == '\'') {
            ++url_start;
            url_end = strchr(url_start, '\'');
            if ((!url_end) || (url_end > tag_end))
              url_end = NULL;
          }
          else {
            url_end = strchr(url_start, ' ');
            if ((!url_end) || (url_end > tag_end))
              url_end = tag_end;
          }
          if (!url_end)
            return false; // Broken HTML
          std::string url(url_start, url_end - url_start);
          url = uri_unencode(url);
          if (url.find("://") != std::string::npos) {
            URL u(url);
            std::string b = base.str();
            if (b[b.size() - 1] != '/')
              b += '/';
            if (u.str().substr(0, b.size()) == b)
              url = u.str().substr(b.size());
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

  static DataStatus do_stat(const std::string& path, ClientHTTP& client, FileInfo& file) {
    PayloadRaw request;
    PayloadRawInterface *inbuf;
    HTTPClientInfo info;
    info.lastModified = (time_t)(-1);
    // Do HEAD to obtain some metadata
    MCC_Status r = client.process("HEAD", path, &request, &info, &inbuf);
    if (inbuf) delete inbuf;
    // TODO: handle redirects
    if ((!r) || (info.code != 200)) {
      if(info.code == 404) return DataStatus::StatNotPresentError;
      return DataStatus::StatError;
    }
    // Fetch known metadata
    file.SetMetaData("path", path);
    std::string type = info.type;
    std::string::size_type pos = type.find(';');
    if (pos != std::string::npos) type = type.substr(0, pos);

    // Treat every html as potential directory/set of links
    if(type == "text/html") {
      file.SetType(FileInfo::file_type_dir);
      file.SetMetaData("type", "dir");
    } else {
      file.SetType(FileInfo::file_type_file);
      file.SetMetaData("type", "file");
    }
    if(info.size != (uint64_t)(-1)) {
      file.SetSize(info.size);
      file.SetMetaData("size", tostring(info.size));
    }
    if(info.lastModified != (time_t)(-1)) {
      file.SetCreated(info.lastModified);
      file.SetMetaData("mtime", info.lastModified.str());
    }
    // Not sure
    if(!info.location.empty()) file.AddURL(info.location);
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Stat(FileInfo& file, DataPointInfoType verb) {
    // verb is not used
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    ClientHTTP client(cfg, url, usercfg.Timeout());

    DataStatus r = do_stat(url.FullPathURIEncoded(), client, file);
    if(!r) return r;
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
    if(file.CheckCreated()) {
      created = file.GetCreated();
      logger.msg(VERBOSE, "Stat: obtained modification time %s", created.str());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    if (transfers_started.get() != 0) return DataStatus::ListError;
    URL curl = url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    {
      ClientHTTP client(cfg, curl, usercfg.Timeout());
      FileInfo file;
      DataStatus r = do_stat(curl.FullPathURIEncoded(), client, file);
      if(r) {
        if(file.CheckSize()) size = file.GetSize();
        if(file.CheckCreated()) created = file.GetCreated();
        if(file.GetType() != FileInfo::file_type_dir) return DataStatus::ListError;
      }
    }

    DataBuffer buffer;

    // TODO: Reuse connection
    if (!StartReading(buffer)) return DataStatus::ListError;

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    // TODO: limit size
    while (buffer.for_write() || !buffer.eof_read()) {
      if (buffer.for_write(handle, length, offset, true)) {
        result.append(buffer[handle], length);
        buffer.is_written(handle);
      }
    }

    if (!StopReading()) return DataStatus::ListError;

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
    if (titlestart != std::string::npos && titleend != std::string::npos)
      title = result.substr(titlestart, titleend - titlestart + 1);

    if (is_body) {
      html2list(result.c_str(), url, files);
      if(verb & (INFO_TYPE_TYPE | INFO_TYPE_TIMES | INFO_TYPE_CONTENT)) {
        for(std::list<FileInfo>::iterator f = files.begin(); f != files.end(); ++f) {
          URL furl(url.str()+'/'+(f->GetName()));
          /* for future use
          if((furl.Host() != curl.Host()) ||
             (furl.Port() != curl.Port()) ||
             (furl.Protocol() != curl.Protocol())) {
            curl = furl;
            delete client;
            client = new ClientHTTP(cfg, curl, usercfg.Timeout());
          }
          */
          ClientHTTP client(cfg, curl, usercfg.Timeout());
          do_stat(furl.FullPathURIEncoded(), client, *f);
        }
      }
    }
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartReading(DataBuffer& buffer) {
    if (transfers_started.get() != 0)
      return DataStatus::ReadStartError;
    int transfer_streams = 1;
    DataPointHTTP::buffer = &buffer;
    if (chunks)
      delete chunks;
    chunks = new ChunkControl;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    transfer_lock.lock();
    transfers_tofinish = 0;
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url, usercfg.Timeout());
      if (!CreateThreadFunction(&read_thread, info, &transfers_started)) {
        delete info;
      } else {
        ++transfers_tofinish;
      }
    }
    if (!transfers_tofinish) {
      transfer_lock.unlock();
      StopReading();
      return DataStatus::ReadStartError;
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopReading() {
    if (!buffer) return DataStatus::ReadStopError;
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
    if (transfers_started.get() != 0) return DataStatus::WriteStartError;
    int transfer_streams = 1;
    DataPointHTTP::buffer = &buffer;
    if (chunks)
      delete chunks;
    chunks = new ChunkControl;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    transfer_lock.lock();
    transfers_tofinish = 0;
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url, usercfg.Timeout());
      if (!CreateThreadFunction(&write_thread, info, &transfers_started)) {
        delete info;
      } else {
        ++transfers_tofinish;
      }
    }
    if (!transfers_tofinish) {
      transfer_lock.unlock();
      StopWriting();
      return DataStatus::WriteStartError;
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopWriting() {
    if (!buffer) return DataStatus::WriteStopError;
    while (transfers_started.get()) {
      transfers_started.wait(); // Just in case
    }
    if (chunks)
      delete chunks;
    chunks = NULL;
    transfers_tofinish = 0;
    if (buffer->error_write()) {
      buffer = NULL;
      return DataStatus::WriteError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Check() {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    ClientHTTP client(cfg, url, usercfg.Timeout());
    PayloadRaw request;
    PayloadRawInterface *inbuf;
    HTTPClientInfo info;
    MCC_Status r = client.process("GET", url.FullPathURIEncoded(), 0, 15,
                                  &request, &info, &inbuf);
    PayloadRawInterface::Size_t logsize = 0;
    if (inbuf){
      logsize = inbuf->Size();
      delete inbuf;
    }
    if ((!r) || ((info.code != 200) && (info.code != 206))) return DataStatus::CheckError;
    size = logsize;
    logger.msg(VERBOSE, "Check: obtained size %llu", size);
    created = info.lastModified;
    logger.msg(VERBOSE, "Check: obtained modification time %s", created.str());
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Remove() {
    return DataStatus::DeleteError;
  }

  void DataPointHTTP::read_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    point.transfer_lock.lock();
    point.transfer_lock.unlock();
    ClientHTTP *client = info.client;
    URL client_url = point.url;
    bool transfer_failure = false;
    int retries = 0;
    std::string path = point.CurrentLocation().FullPathURIEncoded();
    for (;;) {
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      // get first buffer
      if (!point.buffer->for_read(transfer_handle, transfer_size, true))
        // No transfer buffer - must be failure or close initiated externally
        break;
      uint64_t transfer_offset = 0;
      uint64_t chunk_length = transfer_size;
      if (!(point.chunks->Get(transfer_offset, chunk_length)))
        // No more chunks to transfer - quit this thread.
        break;
      uint64_t transfer_end = transfer_offset + chunk_length - 1;
      // Read chunk
      HTTPClientInfo transfer_info;
      PayloadRaw request;
      PayloadRawInterface *inbuf;
      MCC_Status r = client->process("GET", path, transfer_offset,
                                     transfer_end, &request, &transfer_info,
                                     &inbuf);
      if (!r) {
        // Failed to transfer chunk - retry.
        // 10 times in a row seems to be reasonable number
        // TODO: mark failure?
        // TODO: report failure.
        if ((++retries) > 10) {
          transfer_failure = true;
          break;
        }
        // Return buffer
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf) delete inbuf;
        // Recreate connection
        delete client;
        client = NULL;
        MCCConfig cfg;
        point.usercfg.ApplyToConfig(cfg);
        client = new ClientHTTP(cfg, client_url, point.usercfg.Timeout());
        continue;
      }
      if (transfer_info.code == 416) { // EOF
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf)
          delete inbuf;
        // TODO: report file size to chunk control
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
        delete client;
        client = NULL;
        MCCConfig cfg;
        point.usercfg.ApplyToConfig(cfg);
        client_url = transfer_info.location;
        logger.msg(VERBOSE,"Redirecting to %s",transfer_info.location);
        if(client_url && (
            (client_url.Protocol() == "http") ||
            (client_url.Protocol() == "https") ||
            (client_url.Protocol() == "httpg"))) {
          client = new ClientHTTP(cfg, client_url, point.usercfg.Timeout());
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
          if ((++retries) <= 10) continue;
        }
        logger.msg(VERBOSE,"HTTP failure %u - %s",transfer_info.code,transfer_info.reason);
        transfer_failure = true;
        break;
      }
      // pick up usefull information from HTTP header
      point.created = transfer_info.lastModified;
      retries = 0;
      bool whole = (inbuf && (((transfer_info.size == inbuf->Size() &&
                               (inbuf->BufferPos(0) == 0))) ||
                    inbuf->Size() == -1));
      // Temporary solution - copy data between buffers
      point.transfer_lock.lock();
      point.chunks->Unclaim(transfer_offset, chunk_length);
      uint64_t transfer_pos = 0;
      for (unsigned int n = 0;; ++n) {
        if (!inbuf) break;
        char *buf = inbuf->Buffer(n);
        if (!buf) break;
        uint64_t pos = inbuf->BufferPos(n);
        unsigned int length = inbuf->BufferSize(n);
        transfer_pos = inbuf->BufferPos(n) + inbuf->BufferSize(n);
        // In general case returned chunk may be of different size than
        // requested
        for (; length;) {
          if (transfer_handle == -1) {
            // Get transfer buffer if needed
            transfer_size = 0;
            point.transfer_lock.unlock();
            if (!point.buffer->for_read(transfer_handle, transfer_size,
                                        true)) {
              // No transfer buffer - must be failure or close initiated
              // externally
              point.transfer_lock.lock();
              break;
            }
            point.transfer_lock.lock();
          }
          unsigned int l = length;
          if (l > transfer_size)
            l = transfer_size;
          char *buf_ = (*point.buffer)[transfer_handle];
          memcpy(buf_, buf, l);
          point.buffer->is_read(transfer_handle, l, pos);
          point.chunks->Claim(pos, l);
          length -= l;
          pos += l;
          buf += l;
          transfer_handle = -1;
        }
      }
      if (transfer_handle != -1)
        point.buffer->is_read(transfer_handle, 0, 0);
      if (inbuf)
        delete inbuf;
      // If server returned chunk which is not overlaping requested one - seems
      // like server has nothing to say any more.
      if (transfer_pos <= transfer_offset)
        whole = true;
      point.transfer_lock.unlock();
      if (whole)
        break;
    }
    point.transfer_lock.lock();
    --(point.transfers_tofinish);
    if (transfer_failure) point.buffer->error_read(true);
    if (point.transfers_tofinish == 0) {
      // TODO: process/report failure?
      point.buffer->eof_read(true);
    }
    if (client) delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

  void DataPointHTTP::write_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    point.transfer_lock.lock();
    point.transfer_lock.unlock();
    ClientHTTP *client = info.client;
    bool transfer_failure = false;
    int retries = 0;
    std::string path = point.CurrentLocation().FullPathURIEncoded();
    for (;;) {
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
      PayloadRawInterface *response;
      MCC_Status r = client->process("PUT", path, &request, &transfer_info,
                                     &response);
      if (response)
        delete response;
      if (!r) {
        // Failed to transfer chunk - retry.
        // 10 times in a row seems to be reasonable number
        // TODO: mark failure?
        // TODO: report failure.
        if ((++retries) > 10) {
          transfer_failure = true;
          break;
        }
        // Return buffer
        point.buffer->is_notwritten(transfer_handle);
        // Recreate connection
        delete client;
        client = NULL;
        MCCConfig cfg;
        point.usercfg.ApplyToConfig(cfg);
        client = new ClientHTTP(cfg, point.url, point.usercfg.Timeout());
        continue;
      }
      if ((transfer_info.code != 201) &&
          (transfer_info.code != 200) &&
          (transfer_info.code != 204)) {  // HTTP error - retry?
        point.buffer->is_notwritten(transfer_handle);
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504))
          if ((++retries) <= 10)
            continue;
        transfer_failure = true;
        break;
      }
      retries = 0;
      point.buffer->is_written(transfer_handle);
    }
    point.transfer_lock.lock();
    --(point.transfers_tofinish);
    if (transfer_failure) point.buffer->error_write(true);
    if (point.transfers_tofinish == 0) {
      // TODO: process/report failure?
      point.buffer->eof_write(true);
      if ((!(point.buffer->error())) && (point.buffer->eof_position() == 0))
        // Zero size data was trasfered - must send at least one empty packet
        for (;;) {
          HTTPClientInfo transfer_info;
          PayloadMemConst request(NULL, 0, 0, 0);
          PayloadRawInterface *response;
          MCC_Status r = client->process("PUT", path, &request, &transfer_info,
                                         &response);
          if (response)
            delete response;
          if (!r) {
            if ((++retries) > 10) {
              point.buffer->error_write(true);
              break;
            }
            // Recreate connection
            delete client;
            client = NULL;
            MCCConfig cfg;
            point.usercfg.ApplyToConfig(cfg);
            client = new ClientHTTP(cfg, point.url, point.usercfg.Timeout());
            continue;
          }
          if ((transfer_info.code != 201) &&
              (transfer_info.code != 200) &&
              (transfer_info.code != 204)) {  // HTTP error - retry?
            if ((transfer_info.code == 500) ||
                (transfer_info.code == 503) ||
                (transfer_info.code == 504))
              if ((++retries) <= 10)
                continue;
            point.buffer->error_write(true);
            break;
          }
          break;
        }
    }
    if (client) delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

  bool DataPointHTTP::SetURL(const URL& url) {
    if(url.Protocol() != this->url.Protocol()) return false;
    if(url.Host() != this->url.Host()) return false;
    if(url.Port() != this->url.Port()) return false;
    this->url = url;
    return true;
  }


} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "http", "HED:DMC", "HTTP or HTTP over SSL (https)", 0, &Arc::DataPointHTTP::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
