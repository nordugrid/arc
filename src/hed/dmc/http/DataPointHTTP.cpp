#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <unistd.h>

#include <arc/Logger.h>
#include <arc/message/PayloadRaw.h>
#include <arc/data/DataBufferPar.h>
#include <arc/misc/ClientInterface.h>
#include "DataPointHTTP.h"

namespace Arc {

  Logger DataPointHTTP::logger(DataPoint::logger, "HTTP");

  typedef struct {
    DataPointHTTP* point;
    ClientHTTP* client;
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
    bool Get(uint64_t &start, uint64_t &length);
    void Claim(uint64_t start, uint64_t length);
    void Unclaim(uint64_t start, uint64_t length);
  };

  class PayloadMemConst: public PayloadRawInterface {
   private:
    char* buffer_;
    uint64_t begin_;
    unsigned int end_;
    uint64_t size_;
   public:
    PayloadMemConst(void *buffer,
                    uint64_t offset,
                    unsigned int length,
                    uint64_t size = 0) : buffer_((char*)buffer),
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
    virtual char* Insert(int /* pos */ = 0,int /* size */ = 0) {
      return NULL;
    }
    virtual char* Insert(const char* /* s */ ,int /* pos */ = 0,int /* size */ = 0) {
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
    chunk_t chunk = { 0, UINT64_MAX };
    chunks_.push_back(chunk);
  }

  bool ChunkControl::Get(uint64_t& start,uint64_t& length) {
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
    else {
      c->start += length;
    }
    lock_.unlock();
    return true;
  }

  void ChunkControl::Claim(uint64_t start,uint64_t length) {
    if (length == 0)
      return;
    uint64_t end = start+length;
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
        c->start=end;
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

  void ChunkControl::Unclaim(uint64_t start,uint64_t length) {
    if (length == 0)
      return;
    uint64_t end = start+length;
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
          std::list<chunk_t>::iterator c_ = c; ++c_;
          while (c_ != chunks_.end()) {
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
        }
        lock_.unlock();
        return;
      }
      if ((start <= c->start) && (end >= c->end)) {
        c->start = start;
        if (end > c->end) {
          c->end=end;
          std::list<chunk_t>::iterator c_ = c; ++c_;
          while (c_ != chunks_.end()) {
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
        }
        lock_.unlock();
        return;
      }
      if (end < c->start) {
        chunk_t chunk = {start, end};
        chunks_.insert(c, chunk);
        lock_.unlock();
        return;
      }
    }
    lock_.unlock();
  }

  // Globus legacy
  static void get_environment_credentials(Arc::BaseConfig& cfg) {
    if (getenv("X509_USER_CERT"))
      cfg.AddCertificate(getenv("X509_USER_CERT"));
    if (getenv("X509_USER_KEY"))
      cfg.AddPrivateKey(getenv("X509_USER_KEY"));
    if (getenv("X509_USER_PROXY"))
      cfg.AddProxy(getenv("X509_USER_PROXY"));
    if (getenv("X509_CERT_DIR"))
      cfg.AddCADir(getenv("X509_CERT_DIR"));
  }

  DataPointHTTP::DataPointHTTP(const URL& url) : DataPointDirect(url),
                                                 chunks(NULL),
                                                 transfers_started(0), 
                                                 transfers_finished(0) {}

  DataPointHTTP::~DataPointHTTP() {
    StopReading();
    StopWriting();
    if (chunks)
      delete chunks;
  }

  DataStatus DataPointHTTP::ListFiles(std::list<FileInfo>& files, bool) {

    MCCConfig cfg;
    get_environment_credentials(cfg);
    ClientHTTP client(cfg, url.Host(), url.Port(),
                      url.Protocol() == "https", url.str());

    PayloadRaw request;
    PayloadRawInterface* response;

    Arc::HTTPClientInfo info;
    client.process("GET", &request, &info, &response);

    std::list<FileInfo>::iterator f = files.insert(files.end(), url.Path());
    f->SetType(FileInfo::file_type_file);
    f->SetSize(info.size);
    f->SetCreated(info.lastModified);

    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartReading(DataBufferPar& buffer) {
std::cerr<<"DataPointHTTP::StartReading: "<<(unsigned int)this<<std::endl;
    if (transfers_started != 0)
      return DataStatus::ReadStartError;
    int transfer_streams = 1;
    int started = 0;
    DataPointHTTP::buffer = &buffer;
    if (chunks)
      delete chunks;
    chunks = new ChunkControl;
    MCCConfig cfg;
    get_environment_credentials(cfg);
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t* info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url.Host(), url.Port(),
                                    url.Protocol() == "https", url.str());
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
    while(transfers_started < started) {
      transfer_lock.unlock();
      sleep(1);
      transfer_lock.lock();
    }
    transfer_lock.unlock();
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StopReading() {
std::cerr<<"DataPointHTTP::StopReading: "<<(unsigned int)this<<std::endl;
    if (!buffer)
      return DataStatus::ReadStopError;
    if (transfers_finished < transfers_started) {
      buffer->error_read(true);
      transfer_lock.lock();
      while (transfers_finished < transfers_started) {
        transfer_lock.unlock();
        sleep(1);
        transfer_lock.lock();
      }
      transfer_lock.unlock();
      delete chunks;
      chunks = NULL;
    }
    buffer = NULL;
    transfers_finished=0;
    transfers_started=0;
    return DataStatus::Success;
  }

  DataStatus DataPointHTTP::StartWriting(DataBufferPar& buffer,
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
    get_environment_credentials(cfg);
    for (int n = 0; n < transfer_streams; ++n) {
      HTTPInfo_t* info = new HTTPInfo_t;
      info->point = this;
      info->client = new ClientHTTP(cfg, url.Host(), url.Port(),
                                    url.Protocol() == "https", url.str());
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
    while(transfers_started < started) {
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
    if (transfers_finished < transfers_started) {
      buffer->error_write(true);
      transfer_lock.lock();
      while (transfers_finished < transfers_started) {
        transfer_lock.unlock();
        sleep(1);
        transfer_lock.lock();
      }
      transfer_lock.unlock();
      delete chunks;
      chunks = NULL;
    }
    buffer = NULL;
    transfers_finished=0;
    transfers_started=0;
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
std::cerr<<"DataPointHTTP::read_thread: "<<(unsigned int)(info.point)<<std::endl;
    ClientHTTP* client = info.client;
    bool transfer_failure = false;
    point.transfer_lock.lock();
    ++(point.transfers_started);
    point.transfer_lock.unlock();
    for(;;) {
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      // get first buffer
      if (!point.buffer->for_read(transfer_handle,transfer_size,true)) {
        // No transfer buffer - must be failure or close initiated externally
        break;
      }
      uint64_t transfer_offset = 0;
      uint64_t chunk_length = transfer_size;
      if (!(point.chunks->Get(transfer_offset,chunk_length))) {
        // No more chunks to transfer - quit this thread.
        break;
      }
      uint64_t transfer_end = transfer_offset+chunk_length;
      // Read chunk
      Arc::HTTPClientInfo transfer_info;
      PayloadRaw request;
      PayloadRawInterface* inbuf;
      std::string path = point.CurrentLocation().Path();
      path = "/" + path;
      MCC_Status r = client->process("GET",path,transfer_offset,transfer_end,&request,&transfer_info,&inbuf);
      if (!r) {
        // Failed to transfer chunk - retry.
        // TODO: implement internal retry count?
        // TODO: mark failure?
        // TODO: report failure.
        // Return buffer 
        point.buffer->is_read(transfer_handle,0,0);
        point.chunks->Unclaim(transfer_offset,chunk_length);
        if (inbuf)
          delete inbuf;
        // Recreate connection
        delete client;
        client = NULL;
        MCCConfig cfg;
        get_environment_credentials(cfg);
        client=new ClientHTTP(cfg,point.url.Host(),point.url.Port(),point.url.Protocol() == "https",point.url.str());
        continue;
      }
      if (transfer_info.code == 416) { // EOF
        point.buffer->is_read(transfer_handle,0,0);
        point.chunks->Unclaim(transfer_offset,chunk_length);
        if (inbuf)
          delete inbuf;
        // TODO: report file size to chunk control
        break;
      }
      if ((transfer_info.code != 200) &&
         (transfer_info.code != 206)) { // HTTP error - retry?
        point.buffer->is_read(transfer_handle,0,0);
        point.chunks->Unclaim(transfer_offset,chunk_length);
        if (inbuf)
          delete inbuf;
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504)) {
          continue;
        }
        transfer_failure = true;
        break;
      }
      // Temporary solution - copy data between buffers
      point.transfer_lock.lock();
      point.chunks->Unclaim(transfer_offset,chunk_length);
      for (unsigned int n = 0;;++n) {
        char* buf = inbuf->Buffer(n);
        if (!buf)
          break;
        uint64_t pos = inbuf->BufferPos(n);
        unsigned int length = inbuf->BufferSize(n);
        // In general case returned chunk may be of different size than requested
        if (transfer_handle == -1) {
          // Get transfer buffer if needed
          if (!point.buffer->for_read(transfer_handle,transfer_size,true)) {
            // No transfer buffer - must be failure or close initiated externally
            if (inbuf)
              delete inbuf;
            break;
          }
        }
        if (length > transfer_size)
          length = transfer_size;
        // TODO: prevent information loss, so far assuming 
        // returned chunks are not bigger than requested.
        char* buf_ = (*point.buffer)[transfer_handle];
        memcpy(buf_,buf,length);
        point.buffer->is_read(transfer_handle,length,pos);
        point.chunks->Claim(pos,length);
        transfer_handle = -1;
      }
      if (inbuf)
        delete inbuf;
      point.transfer_lock.unlock();
    }
    point.transfer_lock.lock();
    ++(point.transfers_finished);
    if (transfer_failure)
      point.buffer->error_read(true);
    if (point.transfers_finished == point.transfers_started) {
      // TODO: process/report failure?
      point.buffer->eof_read(true);
    }
    if(client)
      delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

  void DataPointHTTP::write_thread(void *arg) {
    HTTPInfo_t& info = *((HTTPInfo_t*)arg);
    DataPointHTTP& point = *(info.point);
    ClientHTTP* client = info.client;
    bool transfer_failure = false;
    point.transfer_lock.lock();
    ++(point.transfers_started);
    point.transfer_lock.unlock();
    for(;;) {
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      unsigned long long int transfer_offset = 0;
      // get first buffer
      if (!point.buffer->for_write(transfer_handle,transfer_size,transfer_offset,true)) {
        // No transfer buffer - must be failure or close initiated externally
        break;
      }
      //uint64_t transfer_offset = 0;
      //uint64_t transfer_end = transfer_offset+transfer_size;
      // Write chunk
      Arc::HTTPClientInfo transfer_info;
      PayloadMemConst request((*point.buffer)[transfer_handle],transfer_offset,transfer_size,
                              point.CheckSize()?point.GetSize():0);
      PayloadRawInterface* response;
      std::string path = point.CurrentLocation().Path();
      path = "/" + path;
      MCC_Status r = client->process("PUT",path,&request,&transfer_info,&response);
      if (response)
        delete response;
      if (!r) {
        // Failed to transfer chunk - retry.
        // TODO: implement internal retry count?
        // TODO: mark failure?
        // TODO: report failure.
        // Return buffer 
        point.buffer->is_notwritten(transfer_handle);
        // Recreate connection
        delete client;
        client = NULL;
        MCCConfig cfg;
        get_environment_credentials(cfg);
        client=new ClientHTTP(cfg,point.url.Host(),point.url.Port(),point.url.Protocol() == "https",point.url.str());
        continue;
      }
      if (transfer_info.code != 200) { // HTTP error - retry?
        point.buffer->is_notwritten(transfer_handle);
        if ((transfer_info.code == 500) ||
            (transfer_info.code == 503) ||
            (transfer_info.code == 504)) {
          continue;
        }
        transfer_failure = true;
        break;
      }
      point.buffer->is_written(transfer_handle);
    }
    point.transfer_lock.lock();
    ++(point.transfers_finished);
    if (transfer_failure)
      point.buffer->error_write(true);
    if (point.transfers_finished == point.transfers_started) {
      // TODO: process/report failure?
      point.buffer->eof_write(true);
    }
    if (client)
      delete client;
    delete &info;
    point.transfer_lock.unlock();
  }

} // namespace Arc
