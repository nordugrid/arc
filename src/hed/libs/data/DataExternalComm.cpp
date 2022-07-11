// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include "DataExternalComm.h"

namespace Arc {

  char const DataExternalComm::ErrorTag = '!';
  char const DataExternalComm::DataStatusTag = 'S';
  char const DataExternalComm::FileInfoTag = 'F';
  char const DataExternalComm::DataChunkTag = 'D';
  char const DataExternalComm::TransferStatusTag = 'T';

  static char const entrySep = '\n';
  static char const itemSep = ',';
  static char const elemSep = '.';

  static char const escapeTag = '~';
  static char const * const escapeChars = "~\n\r,.";

  class EntryFinished: public std::exception {
   public:
    EntryFinished() {};
  };

  char DataExternalComm::InTag(std::istream& instream) {
    char tag = ErrorTag;
    instream.read(&tag, 1);
    if(instream.gcount() != 1) return ErrorTag;
    if(instream.fail()) return ErrorTag;
    return tag;
  }

  char DataExternalComm::InTag(Arc::Run& run, int timeout) {
    char tag = ErrorTag;
    if(run.ReadStdout(timeout, &tag, 1) != 1) return ErrorTag;
    return tag;
  }

  bool DataExternalComm::OutTag(Arc::Run& run, int timeout, char tag) {
   if(run.WriteStdin(timeout, &tag, 1) != 1) return false;
   return true;
  }

  static std::string encode(std::string const& str) {
    return Arc::escape_chars(str, escapeChars, escapeTag, false, Arc::escape_hex);
  }

  static std::string decode(std::string const& str) {
    return unescape_chars(str, escapeTag, Arc::escape_hex);
  }

//  static std::string itemIn(std::istream& instream, char sep = itemSep) {
//    std::string str;
//    std::getline(instream, str, sep);
//    if(instream.fail())
//      throw std::exception();
//    return decode(str);
//  }

  static std::string itemIn(Run& run, int timeout, char sep = itemSep) {
    std::string str;
    while(true) {
      char c;
      if(run.ReadStdout(timeout, &c, 1) != 1)
        throw std::exception();
      if(c == sep)
        break;
      if(c == entrySep)
        throw EntryFinished(); // unexpected higher level separator
      str.push_back(c);
    }
    return decode(str);
  }

  static void itemOut(Run& run, int timeout, std::string const& item, char sep = itemSep) {
    std::string str(encode(item));
    const char *buf = str.c_str();
    int size = str.length();
    while(size > 0) {
      int l = run.WriteStdin(timeout, buf, size);
      if(l <= 0) throw std::exception();
      size -= l; buf += l;
    }
    if(run.WriteStdin(timeout, &sep, 1) != 1) throw std::exception();
  }

//  template<typename T> T itemIn(std::istream& instream, char sep = itemSep) {
//    std::string str;
//    std::getline(instream, str, sep);
//    if(instream.fail())
//      throw std::exception();
//    T item;
//    if(!Arc::stringto<T>(decode(str), item))
//      throw std::exception();
//    return item;
//  }

  template<typename T> static T itemIn(Run& run, int timeout, char sep = itemSep) {
    std::string str(itemIn(run, timeout, sep));
    T item;
    if(!Arc::stringto<T>(str, item))
      throw std::exception();
    return item;
  }

  static void itemOut(std::ostream& outstream, std::string const& item, char sep = itemSep) {
    std::string str(encode(item));
    outstream.write(str.c_str(), str.length());
    outstream.write(&sep, 1);
    if(outstream.fail()) throw std::exception();
  }

  static std::string itemIn(std::istream& instream, char sep = itemSep) {
    std::string str;
    std::getline(instream, str, sep);
    if(instream.fail())
      throw std::exception();
    return decode(str);
  }

  template<typename T> static T itemIn(std::istream& instream, char sep = itemSep) {
    std::string str = itemIn(instream, sep);
    T item;
    if(!Arc::stringto<T>(decode(str), item))
      throw std::exception();
    return item;
  }

  // -------------------------------------------
  // ------------- FileInfo --------------------
  // -------------------------------------------

  // -------------    control    ---------------

  bool DataExternalComm::InEntry(Arc::Run& run, int timeout, Arc::FileInfo& info) {
    try {
      // Reading fixed part
      info.SetName(itemIn(run,timeout));
      info.SetSize(itemIn<unsigned long long int>(run,timeout));
      info.SetCheckSum(itemIn(run,timeout));
      time_t t_sec = itemIn<time_t>(run,timeout,elemSep);
      time_t t_nsec = itemIn<time_t>(run,timeout);
      info.SetModified(Arc::Time(t_sec, t_nsec));
      t_sec = itemIn<time_t>(run,timeout,elemSep);
      t_nsec = itemIn<time_t>(run,timeout);
      info.SetValid(Arc::Time(t_sec, t_nsec));
      info.SetType(static_cast<Arc::FileInfo::Type>(itemIn<int>(run,timeout)));
      info.SetLatency(itemIn(run,timeout));
    } catch(std::exception const&) {
      return false;
    }
    try {
      // Reading variable part
      while(true) {
        std::string item = itemIn(run,timeout);
        if(strncmp(item.c_str(), "url:", 4) == 0) {
        } else if(strncmp(item.c_str(), "meta:", 5) == 0) {
        } else {
          throw std::exception();
        }
      }
      return true;
    } catch(EntryFinished const&) {
      // Expected outcome
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }

  // -------------     child     ---------------

  bool DataExternalComm::OutEntry(std::ostream& outstream, Arc::FileInfo const& info) {
    // fixed part
    outstream<<encode(info.GetName())<<itemSep;
    outstream<<info.GetSize()<<itemSep;
    outstream<<encode(info.GetCheckSum())<<itemSep;
    outstream<<info.GetModified().GetTime()<<elemSep<<info.GetModified().GetTimeNanoseconds()<<itemSep;
    outstream<<info.GetValid().GetTime()<<elemSep<<info.GetValid().GetTimeNanoseconds()<<itemSep;
    outstream<<info.GetType()<<itemSep;
    outstream<<encode(info.GetLatency())<<itemSep;
    // variable part
    std::list<URL> urls = info.GetURLs();
    for(std::list<URL>::const_iterator url = urls.begin(); url != urls.end(); ++url) {
      outstream<<"url:"<<encode(url->fullstr())<<itemSep;
    }
    std::map<std::string, std::string> attrs = info.GetMetaData();
    for(std::map<std::string, std::string>::const_iterator attr = attrs.begin(); attr != attrs.end(); ++attr) {
      outstream<<"meta:"<<encode(attr->first)<<elemSep<<encode(attr->second)<<itemSep;
    }
    outstream<<entrySep;
    outstream.flush();
    return !outstream.fail();
  }

  //  bool DataExternalComm::InEntry(std::istream& instream, Arc::FileInfo& info) {
  //    try {
  //      instream>>info;
  //      return true;
  //    } catch(std::exception const&) {
  //    }
  //    return false;
  //  }


  // -------------------------------------------
  // ------------- DataStatus --------------------
  // -------------------------------------------

  // -------------    control    ---------------

  bool DataExternalComm::InEntry(Arc::Run& run, int timeout, Arc::DataStatus& status) {
    try {
      Arc::DataStatus::DataStatusType statusCode = static_cast<Arc::DataStatus::DataStatusType>(itemIn<int>(run,timeout));
      int errorCode = itemIn<int>(run,timeout);
      std::string desc = itemIn(run,timeout);
      status = Arc::DataStatus(statusCode, errorCode, desc);
      return (InTag(run,timeout) == entrySep); // Check for proper end of entry
    } catch(std::exception const&) {
    }
    return false;
  }

  // -------------     child     ---------------

  bool DataExternalComm::OutEntry(std::ostream& outstream, Arc::DataStatus const& status) {
    outstream<<status.GetStatus()<<itemSep;
    outstream<<status.GetErrno()<<itemSep;
    outstream<<encode(status.GetDesc())<<itemSep;
    outstream<<entrySep;
    outstream.flush();
    return !outstream.fail();
  }


  // -------------------------------------------
  // ---------- TransferStatus -----------------
  // -------------------------------------------

  // -------------    control    ---------------

  bool DataExternalComm::InEntry(Arc::Run& run, int timeout, Arc::DataExternalComm::TransferStatus& status) {
    try {
      status.bytes_count = itemIn<uint64_t>(run,timeout);
      return (InTag(run,timeout) == entrySep); // Check for proper end of entry
    } catch(std::exception const&) {
    }
    return false;
  }

  // -------------     child     ---------------

  bool DataExternalComm::OutEntry(std::ostream& outstream, Arc::DataExternalComm::TransferStatus const& status) {
    outstream<<status.bytes_count<<itemSep;
    outstream<<entrySep;
    outstream.flush();
    return !outstream.fail();
  }


  // -------------------------------------------
  // ----------- Arc::UserConfig ---------------
  // -------------------------------------------

  // Only selected set of information is passsed

  // -------------    control    ---------------

  bool DataExternalComm::OutEntry(Arc::Run& run, int timeout, Arc::UserConfig const& data) {
    try {
      itemOut(run, timeout, Arc::inttostr(data.Timeout()));
      itemOut(run, timeout, data.Verbosity());
      itemOut(run, timeout, data.CredentialString());
      itemOut(run, timeout, data.ProxyPath());
      itemOut(run, timeout, data.CertificatePath());
      itemOut(run, timeout, data.KeyPath());
      itemOut(run, timeout, data.KeyPassword());
      itemOut(run, timeout, data.CACertificatePath());
      itemOut(run, timeout, data.CACertificatesDirectory());
      itemOut(run, timeout, const_cast<Arc::UserConfig&>(data).VOMSESPath());
      return OutTag(run, timeout, entrySep);
    } catch(std::exception const&) {
    }
    return false;
  }

  // -------------     child     ---------------

  bool DataExternalComm::InEntry(std::istream& instream, Arc::UserConfig& data) {
    try {
      data.Timeout(itemIn<int>(instream));
      {
        std::string verbosity = itemIn(instream);
        if(!verbosity.empty()) data.Verbosity(verbosity);
      };
      data.CredentialString(itemIn(instream));
      data.ProxyPath(itemIn(instream));
      data.CertificatePath(itemIn(instream));
      data.KeyPath(itemIn(instream));
      data.KeyPassword(itemIn(instream));
      data.CACertificatePath(itemIn(instream));
      data.CACertificatesDirectory(itemIn(instream));
      data.VOMSESPath(itemIn(instream));
      return (InTag(instream) == entrySep);
    } catch(std::exception const&) {
    }
    return false;
  }


  // -------------------------------------------
  // ------------- DataChunk -------------------
  // -------------------------------------------

  // -------------    control    ---------------

  DataExternalComm::DataChunkExtBuffer::DataChunkExtBuffer() : offset_left(0), size_left(0) {
  }

  bool DataExternalComm::DataChunkExtBuffer::write(Arc::Run& run, int timeout, void const* data, unsigned long long int offset, unsigned long long int size) const {
    try {
      itemOut(run, timeout, Arc::inttostr(offset));
      itemOut(run, timeout, Arc::inttostr(size));
      while(size > 0) {
        int l = run.WriteStdin(timeout, (char const*)data, (size > INT_MAX)?INT_MAX:size);
        if(l <= 0) throw std::exception();
        size -= l; data = (void const*)(((char const*)data) + l);
      }
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }

  bool DataExternalComm::DataChunkExtBuffer::read(Arc::Run& run, int timeout, void* data, unsigned long long int& offset, unsigned long long int& size) {
    try {
      if(size_left == 0) {
        offset_left = itemIn<unsigned long long int>(run, timeout);
        size_left = itemIn<unsigned long long int>(run, timeout);
      }
      if(size > size_left) size = size_left;
      unsigned long long int size_read = 0;
      if(size > 0) size_read = run.ReadStdout(timeout, (char*)data, size);
      size = size_read;
      offset = offset_left;
      offset_left += size_read;
      size_left -= size_read;
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }

  // -------------     child     ---------------

  DataExternalComm::DataChunkClient::DataChunkClient(): data(NULL), data_allocated(false), offset(0), size(0), eof(false) {
  }

  DataExternalComm::DataChunkClient::DataChunkClient(void* data, unsigned long long int offset, unsigned long long int size):
     data((char*)data), data_allocated(false), offset(offset), size(size), eof(false) {
  }

  DataExternalComm::DataChunkClient::DataChunkClient(DataChunkClient& other):
     data(other.data), data_allocated(other.data_allocated), offset(other.offset), size(other.size), eof(other.eof) {
    other.data_allocated = false;
  }

  DataExternalComm::DataChunkClient& DataExternalComm::DataChunkClient::operator=(DataExternalComm::DataChunkClient& other) {
    if(data_allocated) delete[] data;
    data = other.data;
    data_allocated = other.data_allocated;
    offset= other.offset;
    size = other.size;
    eof = other.eof;
    other.data_allocated = false;
    return *this;
  }

  DataExternalComm::DataChunkClient::~DataChunkClient() {
    if(data_allocated) delete[] data;
  }

  DataExternalComm::DataChunkClient& DataExternalComm::DataChunkClient::MakeCopy() {
    if(data_allocated) return *this; // already copied
    if(!data || !size) return *this;
    char* new_data = new char[size];
    std::memcpy(new_data, data, size);
    data = new_data;
    data_allocated = true;
    return *this;
  }

  bool DataExternalComm::DataChunkClient::write(std::ostream& outstream) const {
    try {
      itemOut(outstream, Arc::inttostr(offset));
      itemOut(outstream, Arc::inttostr(size));
      if(size > 0) outstream.write(data, size);
      if(outstream.fail()) throw std::exception();
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }

  bool DataExternalComm::DataChunkClient::read(std::istream& instream) {
    try {
      if(data_allocated) delete[] data;
      data = NULL; data_allocated = false;
      offset = 0; size = 0;
      offset = itemIn<unsigned long long int>(instream);
      size = itemIn<unsigned long long int>(instream);
      if(size > 0) {
        data = new char[size];
        data_allocated = true;
        instream.read(data, size);
        if(static_cast<unsigned long long int>(instream.gcount()) != size) throw std::exception();
      }
      eof = (size == 0);
      return true;
    } catch(std::exception const&) {
    }
    return false;
  }


} // namespace Arc

