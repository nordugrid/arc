// -*- indent-tabs-mode: nil -*-

#include <arc/Run.h>
#include <arc/data/FileInfo.h>
#include <arc/data/DataStatus.h>
#include <arc/UserConfig.h>

namespace ArcDMCGridFTP {

  extern char const ErrorTag;
  extern char const DataStatusTag;
  extern char const FileInfoTag;
  extern char const DataChunkTag;

  template<typename T> bool InEntry(std::istream& instream, T& entry) {
    try {
      instream>>entry;
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }

  template<typename T> void OutEntry(Arc::Run& run, int timeout, T& entry) {
    std::ostringstream ostream;
    ostream<<entry;
    std::string entry_str((std::ostringstream()<<entry).str());
    run.WriteStdin(-1, entry_str.c_str(), entry_str.length());
  }

  char InTag(std::istream& instream);
  char InTag(Arc::Run& run, int timeout);
  bool OutTag(Arc::Run& run, int timeout, char tag);

  bool OutEntry(std::ostream& outstream, Arc::FileInfo const& info);
  bool InEntry(Arc::Run& run, int timeout, Arc::FileInfo& info);

  bool OutEntry(std::ostream& outstream, Arc::DataStatus const& status);
  bool InEntry(Arc::Run& run, int timeout, Arc::DataStatus& status);

  bool OutEntry(Arc::Run& run, int timeout, Arc::UserConfig const& data);
  bool InEntry(std::istream& instream, Arc::UserConfig& data);

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
    DataChunkClient(); // empty
    DataChunkClient(void* data, unsigned long long int offset, unsigned long long int size);
    ~DataChunkClient();
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

  bool OutEntry(Arc::Run& run, int timeout, DataChunkExtBuffer const& data);
  bool InEntry(Arc::Run& run, int timeout, DataChunkExtBuffer& data);
  bool OutEntry(std::ostream& outstream, DataChunkClient const& data);
  bool InEntry(std::istream& instream, DataChunkClient& data);

} // namespace ArcDMCGridFTP

