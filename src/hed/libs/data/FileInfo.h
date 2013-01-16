// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FILEINFO_H__
#define __ARC_FILEINFO_H__

#include <list>
#include <string>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/StringConv.h>

namespace Arc {

  /// FileInfo stores information about files (metadata).
  /**
   * Set/Get methods exist for "standard" metadata such as name, size and
   * modification time, and there is a generic key-value map for
   * protocol-specific attributes. The Set methods always set the corresponding
   * entry in the generic map, so there is no need for a caller make two calls,
   * for example SetSize(1) followed by SetMetaData("size", "1").
   * \ingroup data
   * \headerfile FileInfo.h arc/data/FileInfo.h
   */
  class FileInfo {

  public:

    /// Type of file object.
    enum Type {
      file_type_unknown = 0, ///< Unknown
      file_type_file = 1,    ///< File-type
      file_type_dir = 2      ///< Directory-type
    };

    /// Construct a new FileInfo with optional name (file path).
    FileInfo(const std::string& name = "")
      : name(name),
        size((unsigned long long int)(-1)),
        modified((time_t)(-1)),
        valid((time_t)(-1)),
        type(file_type_unknown),
        latency("") {
      if (!name.empty()) metadata["name"] = name;
    }

    /// Returns the name (file path) of the file.
    const std::string& GetName() const {
      return name;
    }

    /// Returns the last component of the file name (like the "basename" command).
    std::string GetLastName() const {
      std::string::size_type pos = name.rfind('/');
      if (pos != std::string::npos)
        return name.substr(pos + 1);
      else
        return name;
    }

    /// Set name of the file (file path).
    void SetName(const std::string& n) {
      name = n;
      metadata["name"] = n;
    }

    /// Returns the list of file replicas (for index services).
    const std::list<URL>& GetURLs() const {
      return urls;
    }

    /// Add a replica to this file.
    void AddURL(const URL& u) {
      urls.push_back(u);
    }

    /// Check if file size is known.
    bool CheckSize() const {
      return (size != (unsigned long long int)(-1));
    }

    /// Returns file size.
    unsigned long long int GetSize() const {
      return size;
    }

    /// Set file size.
    void SetSize(const unsigned long long int s) {
      size = s;
      metadata["size"] = tostring(s);
    }

    /// Check if checksum is known.
    bool CheckCheckSum() const {
      return (!checksum.empty());
    }

    /// Returns checksum.
    const std::string& GetCheckSum() const {
      return checksum;
    }

    /// Set checksum.
    void SetCheckSum(const std::string& c) {
      checksum = c;
      metadata["checksum"] = c;
    }

    /// Check if modified time is known.
    bool CheckModified() const {
      return (modified != -1);
    }

    /// Returns modified time.
    Time GetModified() const {
      return modified;
    }

    /// Set modified time.
    void SetModified(const Time& t) {
      modified = t;
      metadata["mtime"] = t.str();
    }

    /// Check if validity time is known.
    bool CheckValid() const {
      return (valid != -1);
    }

    /// Returns validity time.
    Time GetValid() const {
      return valid;
    }

    /// Set validity time.
    void SetValid(const Time& t) {
      valid = t;
      metadata["validity"] = t.str();
    }

    /// Check if file type is known.
    bool CheckType() const {
      return (type != file_type_unknown);
    }

    /// Returns file type.
    Type GetType() const {
      return type;
    }

    /// Set file type.
    void SetType(const Type t) {
      type = t;
      if (t == file_type_file) metadata["type"] = "file";
      else if (t == file_type_dir) metadata["type"] = "dir";
    }

    /// Check if access latency is known.
    bool CheckLatency() const {
      return (!latency.empty());
    }

    /// Returns access latency.
    std::string GetLatency() const {
      return latency;
    }

    /// Set access latency.
    void SetLatency(const std::string l) {
      latency = l;
      metadata["latency"] = l;
    }

    /// Returns map of generic metadata.
    std::map<std::string, std::string> GetMetaData() const {
      return metadata;
    }
    
    /// Set an attribute of generic metadata.
    void SetMetaData(const std::string att, const std::string val) {
      metadata[att] = val;
    }
    
    /// Returns true if this file's name is before f's name alphabetically.
    bool operator<(const FileInfo& f) const {
      return (lower(this->name).compare(lower(f.name)) < 0);
    }

    /// Returns true if file name is defined.
    operator bool() const {
      return !name.empty();
    }

    /// Returns true if file name is not defined.
    bool operator!() const {
      return name.empty();
    }

  private:

    std::string name;
    std::list<URL> urls;         // Physical enpoints/URLs.
    unsigned long long int size; // Size of file in bytes.
    std::string checksum;        // Checksum of file.
    Time modified;               // Creation/modification time.
    Time valid;                  // Valid till time.
    Type type;                   // File type - usually file_type_file
    std::string latency;         // Access latenct of file (applies to SRM only)
    std::map<std::string, std::string> metadata; // Generic metadata attribute-value pairs
  };

} // namespace Arc

#endif // __ARC_FILEINFO_H__
