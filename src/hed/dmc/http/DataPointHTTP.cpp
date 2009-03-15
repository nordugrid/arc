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
#include <arc/data/DataBuffer.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/client/ClientInterface.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "DataPointHTTP.h"

namespace Arc {

  Logger DataPointHTTP::logger(DataPoint::logger, "HTTP");

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
    ~ChunkControl(void);
    // Get chunk to be transfered. On input 'length'
    // contains maximal acceptable chunk size.
    bool Get(uint64_t& start, uint64_t& length);
    // Report chunk transfered. It may be _different_
    // from one obtained through Get().
    void Claim(uint64_t start, uint64_t length);
    // Report chunk not transfered. It must be
    // _same_ as one obtained by Get().
    void Unclaim(uint64_t start, uint64_t length);
  };

  class PayloadMemConst
    : public PayloadRawInterface {
  private:
    char *buffer_;
    uint64_t begin_;
    unsigned int end_;
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
    virtual char operator[](int pos) const {
      if (!buffer_)
        return 0;
      if (pos < begin_)
        return 0;
      if (pos >= end_)
        return 0;
      return buffer_[pos - begin_];
    }
    virtual char* Content(int pos = -1) {
      if (!buffer_)
        return NULL;
      if (pos < begin_)
        return NULL;
      if (pos >= end_)
        return NULL;
      return
        buffer_ + (pos - begin_);
    }
    virtual int Size() const {
      return size_;
    }
    virtual char* Insert(int /* pos */ = 0, int /* size */ = 0) {
      return NULL;
    }
    virtual char* Insert(const char* /* s */, int /* pos */ = 0, int /* size */ = 0) {
      return NULL;
    }
    virtual char* Buffer(unsigned int num) {
      if (num != 0)
        return NULL;
      return buffer_;
    }
    virtual int BufferSize(unsigned int num) const {
      if (!buffer_)
        return 0;
      if (num != 0)
        return 0;
      return
        end_ - begin_;
    }
    virtual int BufferPos(unsigned int num) const {
      if (!buffer_)
        return 0;
      if (num != 0)
        return 0;
      return begin_;
    }
    virtual bool Truncate(unsigned int /* size */) {
      return false;
    }
  };

  ChunkControl::ChunkControl(uint64_t /* size */) {
    chunk_t chunk = {
      0, UINT64_MAX
    };
    chunks_.push_back(chunk);
  }

  ChunkControl::~ChunkControl(void) {}

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

  DataPointHTTP::DataPointHTTP(const URL& url)
    : DataPointDirect(url),
      chunks(NULL),
      transfers_started(0),
      transfers_finished(0) {}

  DataPointHTTP::~DataPointHTTP() {
    StopReading();
    StopWriting();
    if (chunks)
      delete chunks;
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
          if (url.find("://") != std::string::npos) {
            URL u(url);
            std::string b = base.str();
            if (b[b.size() - 1] != '/')
              b += '/';
            if (u.str().substr(0, b.size()) == b)
              url = u.str().substr(b.size());
          }
          if (url[0] != '?' && url[0] != '/')
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
      pos = tag_end + 1;
    }
    return true;
  }

  DataStatus DataPointHTTP::ListFiles(std::list<FileInfo>& files, bool, bool) {

    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    ClientHTTP client(cfg, url);

    PayloadRaw request;
    PayloadRawInterface *response = NULL;
    HTTPClientInfo info;
    MCC_Status status = client.process("GET", &request, &info, &response);
    if (!response)
      return DataStatus::ListError;
    delete response;
    if (!status)
      return DataStatus::ListError;

    std::string type = info.type;
    std::string::size_type pos = type.find(';');
    if (pos != std::string::npos)
      type = type.substr(0, pos);

    if (type.empty() || (strcasecmp(type.c_str(), "text/html") == 0)) {

      DataBuffer buffer;

      if (!StartReading(buffer))
        return DataStatus::ListError;

      int handle;
      unsigned int length;
      unsigned long long int offset;
      std::string result;

      while (buffer.for_write() || !buffer.eof_read())
        if (buffer.for_write(handle, length, offset, true)) {
          result.append(buffer[handle], length);
          buffer.is_written(handle);
        }

      if (!StopReading())
        return DataStatus::ListError;

      std::string::size_type tagstart = 0;
      std::string::size_type tagend = 0;
      std::string::size_type titlestart = std::string::npos;
      std::string::size_type titleend = std::string::npos;
      do {
        tagstart = result.find('<', tagend);
        if (tagstart == std::string::npos)
          break;
        tagend = result.find('>', tagstart);
        if (tagend == std::string::npos)
          break;
        std::string tag = result.substr(tagstart + 1, tagend - tagstart - 1);
        if (strcasecmp(tag.c_str(), "title") == 0)
          titlestart = tagend + 1;
        if (strcasecmp(tag.c_str(), "/title") == 0)
          titleend = tagstart - 1;
      } while (titlestart == std::string::npos || titleend == std::string::npos);

      std::string title;
      if (titlestart != std::string::npos && titleend != std::string::npos)
        title = result.substr(titlestart, titleend - titlestart + 1);

      // should maybe find a better way to do this...
      if (title.substr(0, 10) == "Index of /" ||
          title.substr(0, 5) == "ARex:")
        html2list(result.c_str(), url, files);
      else {
        std::list<FileInfo>::iterator f = files.insert(files.end(), url.FullPath());
        f->SetType(FileInfo::file_type_file);
        f->SetSize(info.size);
        f->SetCreated(info.lastModified);
      }
    }
    else {
      std::list<FileInfo>::iterator f = files.insert(files.end(), url.FullPath());
      f->SetType(FileInfo::file_type_file);
      f->SetSize(info.size);
      f->SetCreated(info.lastModified);
    }

    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartReading(DataBuffer& buffer) {
    if (transfers_started != 0)
      return DataStatus::ReadStartError;
    int transfer_streams = 1;
    int started = 0;
    DataPointHTTP::buffer = &buffer;
    if (chunks)
      delete chunks;
    chunks = new ChunkControl;
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url);
      if (!CreateThreadFunction(&read_thread, info))
        delete info;
      else
        ++started;
    }
    if (!started) {
      StopReading();
      return DataStatus::ReadStartError;
    }
    transfer_lock.lock();
    while (transfers_started < started) {
      transfer_lock.unlock();
      sleep(1);
      transfer_lock.lock();
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopReading() {
    if (!buffer)
      return DataStatus::ReadStopError;
    transfer_lock.lock();
    if (transfers_finished < transfers_started) {
      buffer->error_read(true);
      while (transfers_finished < transfers_started) {
        transfer_lock.unlock();
        sleep(1);
        transfer_lock.lock();
      }
    }
    transfer_lock.unlock();
    if (chunks)
      delete chunks;
    chunks = NULL;
    transfers_finished = 0;
    transfers_started = 0;
    if (buffer->error_read()) {
      buffer = NULL;
      return DataStatus::ReadError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartWriting(DataBuffer& buffer,
                                         DataCallback*) {
    if (transfers_started != 0)
      return DataStatus::WriteStartError;
    int transfer_streams = 1;
    int started = 0;
    DataPointHTTP::buffer = &buffer;
    if (chunks)
      delete chunks;
    chunks = new ChunkControl;
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t *info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url);
      if (!CreateThreadFunction(&write_thread, info))
        delete info;
      else
        ++started;
    }
    if (!started) {
      StopWriting();
      return DataStatus::WriteStartError;
    }
    transfer_lock.lock();
    while (transfers_started < started) {
      transfer_lock.unlock();
      sleep(1);
      transfer_lock.lock();
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopWriting() {
    if (!buffer)
      return DataStatus::WriteStopError;
    transfer_lock.lock();
    if (transfers_finished < transfers_started) {
      buffer->error_write(true);
      while (transfers_finished < transfers_started) {
        transfer_lock.unlock();
        sleep(1);
        transfer_lock.lock();
      }
    }
    transfer_lock.unlock();
    if (chunks)
      delete chunks;
    chunks = NULL;
    transfers_finished = 0;
    transfers_started = 0;
    if (buffer->error_write()) {
      buffer = NULL;
      return DataStatus::WriteError;
    }
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Check() {
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::Remove() {
    return DataStatus::DeleteError;
  }

  void DataPointHTTP::read_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    ClientHTTP *client = info.client;
    bool transfer_failure = false;
    int retries = 0;
    point.transfer_lock.lock();
    ++(point.transfers_started);
    point.transfer_lock.unlock();
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
      std::string path = point.CurrentLocation().FullPath();
      path = "/" + path;
      MCC_Status r = client->process("GET", path, transfer_offset, transfer_end, &request, &transfer_info, &inbuf);
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
        if (inbuf)
          delete inbuf;
        // Recreate connection
        delete client;
        client = NULL;
        MCCConfig cfg;
        if (!point.proxyPath.empty())
          cfg.AddProxy(point.proxyPath);
        if (!point.certificatePath.empty())
          cfg.AddCertificate(point.certificatePath);
        if (!point.keyPath.empty())
          cfg.AddPrivateKey(point.keyPath);
        if (!point.caCertificatesDir.empty())
          cfg.AddCADir(point.caCertificatesDir);
        client = new ClientHTTP(cfg, point.url);
        continue;
      }
      retries = 0;
      if (transfer_info.code == 416) { // EOF
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf)
          delete inbuf;
        // TODO: report file size to chunk control
        break;
      }
      if ((transfer_info.code != 200) &&
          (transfer_info.code != 206)) { // HTTP error - retry?
        point.buffer->is_read(transfer_handle, 0, 0);
        point.chunks->Unclaim(transfer_offset, chunk_length);
        if (inbuf)
          delete inbuf;
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504))
          continue;
        transfer_failure = true;
        break;
      }
      bool whole = (inbuf &&
                    ((transfer_info.size == inbuf->Size()) && (inbuf->BufferPos(0) == 0)) ||
                    (inbuf->Size() == -1)
                    );
      // Temporary solution - copy data between buffers
      point.transfer_lock.lock();
      point.chunks->Unclaim(transfer_offset, chunk_length);
      uint64_t transfer_pos = 0;
      for (unsigned int n = 0;; ++n) {
        if (!inbuf)
          break;
        char *buf = inbuf->Buffer(n);
        if (!buf)
          break;
        uint64_t pos = inbuf->BufferPos(n);
        unsigned int length = inbuf->BufferSize(n);
        transfer_pos = inbuf->BufferPos(n) + inbuf->BufferSize(n);
        // In general case returned chunk may be of different size than requested
        for (; length;) {
          if (transfer_handle == -1) {
            // Get transfer buffer if needed
            transfer_size = 0;
            if (!point.buffer->for_read(transfer_handle, transfer_size, true))
              // No transfer buffer - must be failure or close initiated externally
              break;
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
      //  If server returned chunk which is not overlaping requested one - seems
      // like server has nothing to say any more.
      if (transfer_pos <= transfer_offset)
        whole = true;
      point.transfer_lock.unlock();
      if (whole)
        break;
    }
    point.transfer_lock.lock();
    ++(point.transfers_finished);
    if (transfer_failure)
      point.buffer->error_read(true);
    if (point.transfers_finished == point.transfers_started)
      // TODO: process/report failure?
      point.buffer->eof_read(true);
    if (client)
      delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

  void DataPointHTTP::write_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    ClientHTTP *client = info.client;
    bool transfer_failure = false;
    int retries = 0;
    point.transfer_lock.lock();
    ++(point.transfers_started);
    point.transfer_lock.unlock();
    for (;;) {
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      unsigned long long int transfer_offset = 0;
      // get first buffer
      if (!point.buffer->for_write(transfer_handle, transfer_size, transfer_offset, true))
        // No transfer buffer - must be failure or close initiated externally
        break;
      //uint64_t transfer_offset = 0;
      //uint64_t transfer_end = transfer_offset+transfer_size;
      // Write chunk
      HTTPClientInfo transfer_info;
      PayloadMemConst request((*point.buffer)[transfer_handle], transfer_offset, transfer_size,
                              point.CheckSize() ? point.GetSize() : 0);
      PayloadRawInterface *response;
      std::string path = point.CurrentLocation().FullPath();
      path = "/" + path;
      MCC_Status r = client->process("PUT", path, &request, &transfer_info, &response);
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
        if (!point.proxyPath.empty())
          cfg.AddProxy(point.proxyPath);
        if (!point.certificatePath.empty())
          cfg.AddCertificate(point.certificatePath);
        if (!point.keyPath.empty())
          cfg.AddPrivateKey(point.keyPath);
        if (!point.caCertificatesDir.empty())
          cfg.AddCADir(point.caCertificatesDir);
        client = new ClientHTTP(cfg, point.url);
        continue;
      }
      retries = 0;
      if ((transfer_info.code != 201) &&
          (transfer_info.code != 200) &&
          (transfer_info.code != 204)) {  // HTTP error - retry?
        point.buffer->is_notwritten(transfer_handle);
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504))
          continue;
        transfer_failure = true;
        break;
      }
      point.buffer->is_written(transfer_handle);
    }
    point.transfer_lock.lock();
    ++(point.transfers_finished);
    if (transfer_failure)
      point.buffer->error_write(true);
    if (point.transfers_finished == point.transfers_started)
      // TODO: process/report failure?
      point.buffer->eof_write(true);
    if (client)
      delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

} // namespace Arc
