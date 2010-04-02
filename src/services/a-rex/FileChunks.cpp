#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "FileChunks.h"

namespace ARex {

void FileChunks::Print(void) {
  int n = 0;
  lock.lock();
  for(chunks_t::iterator c = chunks.begin();c!=chunks.end();++c) {
    //Hopi::logger.msg(Arc::DEBUG, "Chunk %u: %u - %u",n,c->first,c->second);
  };
  lock.unlock();
}

void FileChunks::Size(off_t size) {
  lock.lock();
  if(size > FileChunks::size) FileChunks::size = size;
  lock.unlock();
}

FileChunks::FileChunks(FileChunksList& container):list(container),self(container.files.end()),size(0),last_accessed(time(NULL)),refcount(0) {
}

FileChunks::FileChunks(const FileChunks& obj):list(obj.list),self(obj.list.files.end()),size(0),last_accessed(time(NULL)),refcount(0) {
}

FileChunks* FileChunksList::GetStuck(void) {
  if(((int)(time(NULL)-last_timeout)) < timeout) return NULL;
  lock.lock();
  for(std::map<std::string,FileChunks>::iterator f = files.begin();
                    f != files.end();++f) {
    f->second.lock.lock();
    if((f->second.refcount <= 0) && 
       (((int)(time(NULL) - f->second.last_accessed)) >= timeout )) {
      ++(f->second.refcount);
      f->second.lock.unlock();
      lock.unlock();
      return &(f->second);
    }
    f->second.lock.unlock();
  }
  last_timeout=time(NULL);
  lock.unlock();
  return NULL;
}

FileChunks* FileChunksList::GetFirst(void) {
  lock.lock();
  std::map<std::string,FileChunks>::iterator f = files.begin();
  if(f != files.end()) {
    f->second.lock.lock();
    ++(f->second.refcount);
    f->second.lock.unlock();
    lock.unlock();
    return &(f->second);
  };
  lock.unlock();
  return NULL;
}

void FileChunks::Remove(void) {
  lock.lock();
  --refcount;
  if(refcount <= 0) {
    list.lock.lock();
    if(self != list.files.end()) {
      lock.unlock();
      list.files.erase(self);
      list.lock.unlock();
      return;
    }
    list.lock.unlock();
  }
  lock.unlock();
}

FileChunks& FileChunksList::Get(std::string path) {
  lock.lock();
  std::map<std::string,FileChunks>::iterator c = files.find(path);
  if(c == files.end()) {
    c=files.insert(std::pair<std::string,FileChunks>(path,FileChunks(*this))).first;
    c->second.lock.lock();
    c->second.self=c;
  } else {
    c->second.lock.lock();
  }
  ++(c->second.refcount);
  c->second.lock.unlock();
  lock.unlock();
  return (c->second);
}

void FileChunks::Release(void) {
  lock.lock();
  if(chunks.empty()) {
    lock.unlock();
    Remove();
  } else {
    --refcount;
    lock.unlock();
  }
}

void FileChunks::Add(off_t start,size_t csize) {
  off_t end = start+csize;
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

bool FileChunks::Complete(void) {
  lock.lock();
  bool r = ((chunks.size() == 1) &&
            (chunks.begin()->first == 0) &&
            (chunks.begin()->second == size));
  lock.unlock();
  return r;
}

FileChunksList::FileChunksList(void) {
}

FileChunksList::~FileChunksList(void) {
  lock.lock();
  // Not sure
  lock.unlock();
}

} // namespace ARex

