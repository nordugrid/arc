// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAEXTERNALCOMM_H__
#define __ARC_DATAEXTERNALCOMM_H__


#include <arc/Run.h>
#include <arc/data/FileInfo.h>
#include <arc/data/DataStatus.h>
#include <arc/UserConfig.h>

namespace Arc {

  class DataExternalComm {
   public:

    static char const ErrorTag;
    static char const DataStatusTag;
    static char const FileInfoTag;
    static char const DataChunkTag;
    static char const TransferStatusTag;

    template<typename T> static bool InEntry(std::istream& instream, T& entry) {
      try {
        instream>>entry;
        return true;
      } catch(std::exception const&) {
      }
      return false;
    }

    template<typename T> static void OutEntry(Arc::Run& run, int timeout, T& entry) {
      std::ostringstream ostream;
      ostream<<entry;
      std::string entry_str((std::ostringstream()<<entry).str());
      run.WriteStdin(-1, entry_str.c_str(), entry_str.length());
    }

    static char InTag(std::istream& instream);
    static char InTag(Arc::Run& run, int timeout);
    static bool OutTag(Arc::Run& run, int timeout, char tag);

    static bool OutEntry(std::ostream& outstream, Arc::FileInfo const& info);
    static bool InEntry(Arc::Run& run, int timeout, Arc::FileInfo& info);

    class TransferStatus {
     public:
      TransferStatus(unsigned long long int count) : bytes_count(count) {};
      unsigned long long int bytes_count;
    };
    static bool OutEntry(std::ostream& outstream, TransferStatus const& info);
    static bool InEntry(Arc::Run& run, int timeout, TransferStatus& info);

    static bool OutEntry(std::ostream& outstream, Arc::DataStatus const& status);
    static bool InEntry(Arc::Run& run, int timeout, Arc::DataStatus& status);

    static bool OutEntry(Arc::Run& run, int timeout, Arc::UserConfig const& data);
    static bool InEntry(std::istream& instream, Arc::UserConfig& data);

    class DataChunkExtBuffer {
     public:
      DataChunkExtBuffer();
      bool complete() const { return (size_left == 0); }
      bool write(Arc::Run& run, int timeout, void const* data, unsigned long long int offset, unsigned long long int size) const;
      bool read(Arc::Run& run, int timeout, void* data, unsigned long long int& offset, unsigned long long int& size);
     private:
      DataChunkExtBuffer(DataChunkExtBuffer const&);
      DataChunkExtBuffer& operator=(DataChunkExtBuffer const&);
      unsigned long long int offset_left;
      unsigned long long int size_left;
    };

    class DataChunkClient {
     public:
      // Empty buffer
      DataChunkClient();
      // Exteral buffer
      DataChunkClient(void* data, unsigned long long int offset, unsigned long long int size);
      // Move constructor
      DataChunkClient(DataChunkClient& other);
      // Move assignment
      DataChunkClient& operator=(DataChunkClient& other);
      ~DataChunkClient();
      // Copy unallocated buffer into allocated
      DataChunkClient& MakeCopy();
      bool write(std::ostream& outstream) const;
      bool read(std::istream& instream);
      bool getEOF() const { return eof; }
      void* get() const { return data; }
      void* release() { data_allocated = false; return data; }
      unsigned long long int getOffset() const { return offset; }
      unsigned long long int getSize() const { return size; }
     private:
      char* data;
      bool data_allocated;
      unsigned long long int offset;
      unsigned long long int size;
      bool eof;
    };

    // This is wrapper around simple list to make code compatible with old and new compilers.
    class DataChunkClientList {
     public:
      typedef std::list<DataChunkClient*>::iterator chunk_iterator;
      DataChunkClientList() {};
      ~DataChunkClientList() {
        for(chunk_iterator it = chunk_list.begin(); it != chunk_list.end(); ++it) delete *it;
      };
      class iterator: public chunk_iterator {
       public:
        iterator(chunk_iterator const& other):chunk_iterator(other) {};
        DataChunkClient& operator*() { return *(chunk_iterator::operator*()); };
        DataChunkClient* operator->() { return chunk_iterator::operator*(); };
        DataChunkClient* get() { return chunk_iterator::operator*(); };
      };
      //typedef std::list<DataChunkClient*>::iterator iterator;
      void erase(iterator it) { delete it.get(); chunk_list.erase(it); };
      void clear() {
        for(chunk_iterator it = chunk_list.begin(); it != chunk_list.end(); ++it) delete *it;
        chunk_list.clear();
      };
      iterator begin() { return chunk_list.begin(); };
      iterator end() { return chunk_list.end(); };
      bool empty() const { return chunk_list.empty(); };
      std::size_t size() const { return chunk_list.size(); };
      void push_back(DataChunkClient& source) { chunk_list.push_back(new DataChunkClient(source)); };
     private:
      std::list<DataExternalComm::DataChunkClient*> chunk_list;
    };

    static bool OutEntry(Arc::Run& run, int timeout, DataChunkExtBuffer const& data);
    static bool InEntry(Arc::Run& run, int timeout, DataChunkExtBuffer& data);
    static bool OutEntry(std::ostream& outstream, DataChunkClient const& data);
    static bool InEntry(std::istream& instream, DataChunkClient& data);
  }; // class DataExternalComm

} // namespace Arc


#endif // __ARC_DATAEXTERNALCOMM_H__

