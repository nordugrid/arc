#include "DataBufferPar.h"
#include "CheckSum.h"

namespace Arc {

  bool DataBufferPar::set(CheckSum *cksum, unsigned int size, int blocks) {
    lock.lock();
    if(blocks < 0) {
      lock.unlock();
      return false;
    }
    if(bufs != NULL) {
      for(int i = 0; i < bufs_n; i++) {
        if(bufs[i].start)
          free(bufs[i].start);
      }
      free(bufs);
      bufs_n = 0;
      bufs = NULL;
      set_counter++;
      cond.broadcast(); /* make all waiting loops to exit */
    }
    if((size == 0) || (blocks == 0)) {
      lock.unlock();
      return true;
    }
    bufs = (buf_desc *)malloc(sizeof(buf_desc) * blocks);
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    bufs_n = blocks;
    for(int i = 0; i < blocks; i++) {
      bufs[i].start = NULL;
      bufs[i].taken_for_read = false;
      bufs[i].taken_for_write = false;
      bufs[i].size = size;
      bufs[i].used = 0;
      bufs[i].offset = 0;
    }
    checksum = cksum;
    checksum_offset = 0;
    checksum_ready = true;
    if(checksum)
      checksum->start();
    lock.unlock();
    return true;
  }

  DataBufferPar::DataBufferPar(unsigned int size, int blocks) {
    bufs_n = 0;
    bufs = NULL;
    set_counter = 0;
    eof_read_flag = false;
    eof_write_flag = false;
    error_read_flag = false;
    error_write_flag = false;
    error_transfer_flag = false;
    set(NULL, size, blocks);
    eof_pos = 0;
  }

  DataBufferPar::DataBufferPar(CheckSum *cksum, unsigned int size,
                               int blocks) {
    bufs_n = 0;
    bufs = NULL;
    set_counter = 0;
    eof_read_flag = false;
    eof_write_flag = false;
    error_read_flag = false;
    error_write_flag = false;
    error_transfer_flag = false;
    set(cksum, size, blocks);
    eof_pos = 0;
  }

  DataBufferPar::~DataBufferPar() {
    set(NULL, 0, 0);
  }

  bool DataBufferPar::eof_read() {
    return eof_read_flag;
  }

  bool DataBufferPar::eof_write() {
    return eof_write_flag;
  }

  bool DataBufferPar::error_transfer() {
    return error_transfer_flag;
  }

  bool DataBufferPar::error_read() {
    return error_read_flag;
  }

  bool DataBufferPar::error_write() {
    return error_write_flag;
  }

  void DataBufferPar::eof_read(bool eof_) {
    lock.lock();
    if(eof_)
      if(checksum)
        checksum->end();
    eof_read_flag = eof_;
    cond.broadcast();
    lock.unlock();
  }

  void DataBufferPar::eof_write(bool eof_) {
    lock.lock();
    eof_write_flag = eof_;
    cond.broadcast();
    lock.unlock();
  }

  bool DataBufferPar::error() {
    return (error_read_flag || error_write_flag || error_transfer_flag);
  }

  void DataBufferPar::error_read(bool error_) {
    lock.lock();
    // error_read_flag=error_;
    if(error_) {
      if(!(error_write_flag || error_transfer_flag))
        error_read_flag = true;
      if(checksum)
        checksum->end();
      eof_read_flag = true;
    }
    else
      error_read_flag = false;
    cond.broadcast();
    lock.unlock();
  }

  void DataBufferPar::error_write(bool error_) {
    lock.lock();
    // error_write_flag=error_;
    if(error_) {
      if(!(error_read_flag || error_transfer_flag))
        error_write_flag = true;
      eof_write_flag = true;
    }
    else
      error_write_flag = false;
    cond.broadcast();
    lock.unlock();
  }

  bool DataBufferPar::wait_eof_read() {
    lock.lock();
    for(;;) {
      if(eof_read_flag)
        break;
      cond.wait(lock);
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::wait_read() {
    lock.lock();
    for(;;) {
      if(eof_read_flag)
        break;
      if(error_read_flag)
        break;
      cond.wait(lock);
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::wait_eof_write() {
    lock.lock();
    for(;;) {
      if(eof_write_flag)
        break;
      cond.wait(lock);
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::wait_write() {
    lock.lock();
    for(;;) {
      if(eof_write_flag)
        break;
      if(error_write_flag)
        break;
      cond.wait(lock);
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::wait_eof() {
    lock.lock();
    for(;;) {
      if(eof_read_flag && eof_write_flag)
        break;
      cond.wait(lock);
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::cond_wait() {
    // Wait for any event
    int tmp = set_counter;
    bool eof_read_flag_tmp = eof_read_flag;
    bool eof_write_flag_tmp = eof_write_flag;
    /*
       cond.wait(lock);
     */
    int err = -1;
    for(;;) {
      if(!speed.transfer()) {
        if((!(error_read_flag || error_write_flag)) &&
           (!(eof_read_flag && eof_write_flag)))
          error_transfer_flag = true;
      }
      if(eof_read_flag && eof_write_flag) { // there wil be no more events
        lock.unlock();
        Glib::Thread::yield();
        lock.lock();
        return true;
      }
      if(eof_read_flag_tmp != eof_read_flag)
        return true;
      if(eof_write_flag_tmp != eof_write_flag)
        return true;
      if(error())
        return false; // useless to wait for - better fail
      if(set_counter != tmp)
        return false;
      if(err == 0)
        break; // Some event
      int t = 60;
      Glib::TimeVal stime;
      stime.assign_current_time();
      // Using timeout to workaround lost signal
      err = cond.timed_wait(lock, stime + t);
    }
    return true;
  }

  bool DataBufferPar::for_read() {
    if(bufs == NULL)
      return false;
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if((!bufs[i].taken_for_read) && (!bufs[i].taken_for_write) &&
         (bufs[i].used == 0)) {
        lock.unlock();
        return true;
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::for_read(int& handle, unsigned int& length, bool wait) {
    lock.lock();
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    for(;;) {
      if(error()) { /* errors detected/set - any continuation is unusable */
        lock.unlock();
        return false;
      }
      for(int i = 0; i < bufs_n; i++) {
        if((!bufs[i].taken_for_read) && (!bufs[i].taken_for_write) &&
           (bufs[i].used == 0)) {
          if(bufs[i].start == NULL) {
            bufs[i].start = (char *)malloc(bufs[i].size);
            if(bufs[i].start == NULL)
              continue;
          }
          handle = i;
          bufs[i].taken_for_read = true;
          length = bufs[i].size;
          cond.broadcast();
          lock.unlock();
          return true;
        }
      }
      /* suitable block not found - wait for changes or quit */
      if(eof_write_flag) { /* writing side quited, no need to wait */
        lock.unlock();
        return false;
      }
      if(!wait) {
        lock.unlock();
        return false;
      }
      if(!cond_wait()) {
        lock.unlock();
        return false;
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::is_read(char *buf, unsigned int length,
                              unsigned long long int offset) {
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if(bufs[i].start == buf) {
        lock.unlock();
        return is_read(i, length, offset);
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::is_read(int handle, unsigned int length,
                              unsigned long long int offset) {
    lock.lock();
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    if(handle >= bufs_n) {
      lock.unlock();
      return false;
    }
    if(!bufs[handle].taken_for_read) {
      lock.unlock();
      return false;
    }
    if(length > bufs[handle].size) {
      lock.unlock();
      return false;
    }
    bufs[handle].taken_for_read = false;
    bufs[handle].used = length;
    bufs[handle].offset = offset;
    if((offset + length) > eof_pos)
      eof_pos = offset + length;
    /* checksum on the fly */
    if((checksum != NULL) && (offset == checksum_offset)) {
      for(int i = handle; i < bufs_n; i++) {
        if(bufs[i].used != 0) {
          if(bufs[i].offset == checksum_offset) {
            checksum->add(bufs[i].start, bufs[i].used);
            checksum_offset += bufs[i].used;
            i = -1;
            checksum_ready = true;
          }
          else {
            if(checksum_offset < bufs[i].offset)
              checksum_ready = false;
          }
        }
      }
    }
    cond.broadcast();
    lock.unlock();
    return true;
  }

  bool DataBufferPar::for_write() {
    if(bufs == NULL)
      return false;
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if((!bufs[i].taken_for_read) && (!bufs[i].taken_for_write) &&
         (bufs[i].used != 0)) {
        lock.unlock();
        return true;
      }
    }
    lock.unlock();
    return false;
  }

  /* return true + buffer with data,
     return false in case of failure, or eof + no buffers claimed for read */
  bool DataBufferPar::for_write(int& handle, unsigned int& length,
                                unsigned long long int& offset, bool wait) {
    lock.lock();
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    for(;;) {
      if(error()) {/* internal/external errors - no need to continue */
        lock.unlock();
        return false;
      }
      bool have_for_read = false;
      bool have_unused = false;
      unsigned long long int min_offset = (unsigned long long int)(-1);
      handle = -1;
      for(int i = 0; i < bufs_n; i++) {
        if(bufs[i].taken_for_read)
          have_for_read = true;
        if((!bufs[i].taken_for_read) && (!bufs[i].taken_for_write) &&
           (bufs[i].used != 0)) {
          if(bufs[i].offset < min_offset) {
            min_offset = bufs[i].offset;
            handle = i;
          }
        }
        if(bufs[i].taken_for_read || (bufs[i].used == 0))
          have_unused = true;
      }
      if(handle != -1) {
        if((!checksum_ready) && (bufs[handle].offset >= checksum_offset)) {
          /* try to keep buffers as long as possible for checksuming */
          if(have_unused && (!eof_read_flag)) {
            /* still have chances to get that block */
            if(!wait) {
              lock.unlock();
              return false;
            }
            if(!cond_wait()) {
              lock.unlock();
              return false;
            }
            continue;
          }
        }
        bufs[handle].taken_for_write = true;
        length = bufs[handle].used;
        offset = bufs[handle].offset;
        cond.broadcast();
        lock.unlock();
        return true;
      }
      if(eof_read_flag && (!have_for_read)) {
        lock.unlock();
        return false;
      }
      /* suitable block not found - wait for changes or quit */
      if(!wait) {
        lock.unlock();
        return false;
      }
      if(!cond_wait()) {
        lock.unlock();
        return false;
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::is_written(char *buf) {
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if(bufs[i].start == buf) {
        lock.unlock();
        return is_written(i);
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::is_notwritten(char *buf) {
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if(bufs[i].start == buf) {
        lock.unlock();
        return is_notwritten(i);
      }
    }
    lock.unlock();
    return false;
  }

  bool DataBufferPar::is_written(int handle) {
    lock.lock();
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    if(handle >= bufs_n) {
      lock.unlock();
      return false;
    }
    if(!bufs[handle].taken_for_write) {
      lock.unlock();
      return false;
    }
    /* speed control */
    if(!speed.transfer(bufs[handle].used)) {
      if((!(error_read_flag || error_write_flag)) &&
         (!(eof_read_flag && eof_write_flag)))
        error_transfer_flag = true;
    }
    bufs[handle].taken_for_write = false;
    bufs[handle].used = 0;
    bufs[handle].offset = 0;
    cond.broadcast();
    lock.unlock();
    return true;
  }

  bool DataBufferPar::is_notwritten(int handle) {
    lock.lock();
    if(bufs == NULL) {
      lock.unlock();
      return false;
    }
    if(handle >= bufs_n) {
      lock.unlock();
      return false;
    }
    if(!bufs[handle].taken_for_write) {
      lock.unlock();
      return false;
    }
    bufs[handle].taken_for_write = false;
    cond.broadcast();
    lock.unlock();
    return true;
  }

  char *DataBufferPar::operator[](int block) {
    lock.lock();
    if((block < 0) || (block >= bufs_n)) {
      lock.unlock();
      return NULL;
    }
    char *tmp = bufs[block].start;
    lock.unlock();
    return tmp;
  }

  bool DataBufferPar::wait() {
    lock.lock();
    bool res = cond_wait();
    lock.unlock();
    return res;
  }

  bool DataBufferPar::wait_used() {
    lock.lock();
    for(int i = 0; i < bufs_n; i++) {
      if((bufs[i].taken_for_read) || (bufs[i].taken_for_write) ||
         (bufs[i].used != 0)) {
        if(!cond_wait()) {
          lock.unlock();
          return false;
        }
        i = -1;
      }
    }
    lock.unlock();
    return true;
  }

  bool DataBufferPar::checksum_valid() {
    return checksum_ready;
  }

  const CheckSum *DataBufferPar::checksum_object() {
    return checksum;
  }

  unsigned int DataBufferPar::buffer_size() {
    if(bufs == NULL) return 65536;
    unsigned int size = 0;
    for(int i = 0; i < bufs_n; i++) {
      if(size < bufs[i].size)
        size = bufs[i].size;
    }
    return size;
  }

} // namespace Arc
