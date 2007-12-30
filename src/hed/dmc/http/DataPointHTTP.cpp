#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <arc/Logger.h>
#include <arc/message/PayloadRaw.h>
#include <arc/data/DataBufferPar.h>
#include <arc/misc/ClientInterface.h>
#include "DataPointHTTP.h"

namespace Arc {


  class ChunkControl {
   private:
    typedef struct {
      uint64_t start;
      uint64_t end;
    } chunk_t;
    std::list<chunk_t> chunks_;
    Glib::Mutex lock_;
   public:
    ChunkControl(uint64_t size = 0xffffffffffffffffLL);
    bool Get(uint64_t& start,uint64_t& length);
    void Claim(uint64_t start,uint64_t length);
    void Unclaim(uint64_t start,uint64_t length);
  };

  ChunkControl::ChunkControl(uint64_t size) {
    chunk_t chunk = { 0, 0xffffffffffffffffLL};
    chunks_.push_back(chunk);
  }

  bool ChunkControl::Get(uint64_t& start,uint64_t& length) {
    if(length == 0) return false;
    lock_.lock();
    std::list<chunk_t>::iterator c = chunks_.begin();
    if(c == chunks_.end()) { lock_.unlock(); return false; };
    start=c->start; 
    uint64_t l = (c->end)-(c->start);
    if(l <= length) {
      length=l; chunks_.erase(c);
    } else {
      c->start+=(l-length);
    };
    lock_.unlock();
    return true;    
  }

  void ChunkControl::Claim(uint64_t start,uint64_t length) {
    if(length == 0) return;
    uint64_t end = start+length;
    lock_.lock();
    for(std::list<chunk_t>::iterator c = chunks_.begin();c!=chunks_.end();) {
      if(end <= c->start) break;
      if((start <= c->start) && (end >= c->end)) {
        start=c->end;
        length=end-start; 
        c=chunks_.erase(c);
        if(length > 0) continue;
        break;
      };
      if((start > c->start) && (end < c->end)) {
        chunk_t chunk;
        chunk.start=c->start;
        chunk.end=start;
        c->start=end;
        chunks_.insert(c,chunk);
        break;
      };
      if((start <= c->start) && (end < c->end) && (end > c->start)) {
        c->start=end;
        break;
      };
      if((start > c->start) && (start < c->end) && (end >= c->end)) {
        uint64_t start_ = c->end;
        c->end=start;
        start=start_;
        length=end-start; 
        ++c;
        if(length > 0) continue;
        break;
      };
      ++c;
    };
    lock_.unlock();
  }

  void ChunkControl::Unclaim(uint64_t start,uint64_t length) {
    if(length == 0) return;
    uint64_t end = start+length;
    lock_.lock();
    for(std::list<chunk_t>::iterator c = chunks_.begin();c!=chunks_.end();++c) {
      if((end >= c->start) && (end <= c->end)) {
        if(start < c->start) c->start=start;
        lock_.unlock(); return;
      };
      if((start <= c->end) && (start >= c->start)) {
        if(end > c->end) {
          c->end=end;
          std::list<chunk_t>::iterator c_ = c; ++c_;
          for(;c_!=chunks_.end();) {
            if(c->end >= c_->start) {
              if(c_->end >= c->end) { c->end=c_->end; break; };
              c_=chunks_.erase(c_);
            } else { break; };
          };
        };
        lock_.unlock(); return;
      };
      if((start <= c->start) && (end >= c->end)) {
        c->start=start;
        if(end > c->end) {
          c->end=end;
          std::list<chunk_t>::iterator c_ = c; ++c_;
          for(;c_!=chunks_.end();) {
            if(c->end >= c_->start) {
              if(c_->end >= c->end) { c->end=c_->end; break; };
              c_=chunks_.erase(c_);
            } else { break; };
          };
        };
        lock_.unlock(); return;
      };
      if(end < c->start) {
        chunk_t chunk = {start, end};
        chunks_.insert(c,chunk);
        lock_.unlock(); return;
      };
    };
    lock_.unlock();
  }

  Logger DataPointHTTP::logger(DataPoint::logger, "HTTP");

  DataPointHTTP::DataPointHTTP(const URL& url) : DataPointDirect(url), chunks(NULL), threads(0) {}

  DataPointHTTP::~DataPointHTTP() {}

  bool DataPointHTTP::list_files(std::list<FileInfo>& files, bool) {

    MCCConfig cfg;
    ClientHTTP client(cfg, url.Host(), url.Port(),
                      url.Protocol() == "https", url.str());

    PayloadRaw request;
    PayloadRawInterface* response;

    ClientHTTP::Info info;
    client.process("GET", &request, &info, &response);


    std::list<FileInfo>::iterator f = files.insert(files.end(), url.Path());
    f->SetType(FileInfo::file_type_file);
    f->SetSize(info.size);
    f->SetCreated(info.lastModified);

    return true;
  }

  bool DataPointHTTP::start_reading(DataBufferPar& buffer) {
    return false;
  }

  bool DataPointHTTP::start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb) {
    return false;
  }

  typedef struct {
    DataPointHTTP* point;
    ClientHTTP* client;
  } HTTPInfo_t;

  void DataPointHTTP::read_thread(void *arg) {
    DataPointHTTP& point = *(((HTTPInfo_t*)arg)->point);
    ClientHTTP& client = *(((HTTPInfo_t*)arg)->client);
    int handle;
    point.transfer_lock.lock();
    ++(point.threads);
    point.transfer_lock.unlock();
    for(;;) {
      unsigned int transfer_size = 0;
      int transfer_handle = -1;
      // get first buffer
      if(!point.buffer->for_read(transfer_handle,transfer_size,true)) {
        // No transfer buffer - must be failure or close initiated externally
        break;
      };
      uint64_t transfer_offset = 0;
      uint64_t chunk_length = transfer_size;
      if(!(point.chunks->Get(transfer_offset,chunk_length))) {
        // No more chunks to transfer - quit this thread.
        break;
      };
      uint64_t transfer_end = transfer_offset+chunk_length;
      // Read chunk
      ClientHTTP::Info transfer_info;
      PayloadRawInterface* inbuf;
      MCC_Status r = client.process("GET",current_location().Path(),transfer_offset,transfer_end,NULL,&transfer_info,&inbuf);
      if(!r) {
        // Failed to transfer chunk - retry.
        // TODO: implement internal retry count?
        // TODO: mark failure?
        // TODO: report failure.
        // Return buffer 
        point.buffer->is_read(transfer_handle,0,0);
        continue;
      };
      // Temporary solution - copy data between buffers
      point.transfer_lock.lock();
      point.chunks->Unclaim(transfer_offset,chunk_length);
      for(unsigned int n = 0;;++n) {
        char* buf = inbuf->Buffer(n);
        if(!buf) break;
        uint64_t pos = inbuf->BufferPos(n);
        unsigned int length = inbuf->BufferSize(n);
        // In general case returned chunk may be of different size than requested
        if(transfer_handle == -1) {
          // Get transfer buffer if needed
          if(!point.buffer->for_read(transfer_handle,transfer_size,true)) {
            // No transfer buffer - must be failure or close initiated externally
            break;
          };
        };
        if(length > transfer_size) length=transfer_size; 
        // TODO: prevent information loss, so far assuming 
        // returned chunks are not bigger than requested.
        char* buf_ = (*point.buffer)[transfer_handle];
        memcpy(buf_,buf,length);
        point.buffer->is_read(transfer_handle,length,pos);
        point.chunks->Claim(pos,length);
        transfer_handle=-1;
      };
      point.transfer_lock.unlock();
    };
    point.transfer_lock.lock();
    --(point.threads);
    if(point.threads==0) {
      // TODO: process/report failure?
      point.buffer->eof_read(true);
    };
    delete &client;
    point.transfer_lock.unlock();
  }
        
        

/*

      if(s.response().haveLastModified()) {
        istat->point->meta_created(s.response().LastModified());
      };
      // if 0 bytes transfered - behind eof
      odlog(DEBUG)<<"read_thread: check for eof: "<<offset<<" - "<<tstat->offset<<std::endl;
      if(offset == tstat->offset) { failed=false; break; }; // eof
    };
  };
  odlog(DEBUG)<<"read_thread: loop exited"<<std::endl;
  istat->lock.block();
  istat->threads--;
  if(istat->threads==0) {
  odlog(DEBUG)<<"read_thread: last thread: failure: "<<failed<<std::endl;
    if(failed) {
      istat->buffer->error_read(true);
      CHECK_PROXY("read_thread",istat->failure_code);
    };
    istat->buffer->eof_read(true);
  };
  tstat->s=NULL;
  istat->lock.signal_nonblock();
  istat->lock.unblock();
  return NULL;  
}
*/


} // namespace Arc

