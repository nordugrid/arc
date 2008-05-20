#ifndef __ARC_FILEINFO_H__
#define __ARC_FILEINFO_H__

#include <string>
#include <list>

#include <arc/DateTime.h>
#include <arc/URL.h>

namespace Arc {

  /// FileInfo stores information about files (metadata).
  class FileInfo {

  public:

    typedef enum {
      file_type_unknown = 0,
      file_type_file = 1,
      file_type_dir = 2
    } Type;

    FileInfo(const std::string& name = "")
      : name(name),
	size((unsigned long long int)(-1)),
	created((time_t)(-1)),
	valid((time_t)(-1)),
	type(file_type_unknown) {}

    ~FileInfo() {}

    const std::string& GetName() const {
      return name;
    }

    std::string GetLastName() const {
      std::string::size_type pos = name.rfind('/');
      if (pos != std::string::npos)
	return name.substr(pos + 1);
      else
	return name;
    }

    const std::list<URL>& GetURLs() const {
      return urls;
    }

    void AddURL(const URL& u) {
      urls.push_back(u);
    }

    bool CheckSize() const {
      return (size != (unsigned long long int)(-1));
    }

    unsigned long long int GetSize() const {
      return size;
    }

    void SetSize(const unsigned long long int s) {
      size = s;
    }

    bool CheckCheckSum() const {
      return (!checksum.empty());
    }

    const std::string& GetCheckSum() const {
      return checksum;
    }

    void SetCheckSum(const std::string& c) {
      checksum = c;
    }

    bool CheckCreated() const {
      return (created != -1);
    }

    Time GetCreated() const {
      return created;
    }

    void SetCreated(const Time& t) {
      created = t;
    }

    bool CheckValid() const {
      return (valid != -1);
    }

    Time GetValid() const {
      return valid;
    }

    void SetValid(const Time& t) {
      valid = t;
    }

    bool CheckType() const {
      return (type != file_type_unknown);
    }

    Type GetType() const {
      return type;
    }

    void SetType(const Type t) {
      type = t;
    }

  private:

    std::string name;
    std::list<URL> urls;         // Physical enpoints/URLs.
    unsigned long long int size; // Size of file in bytes.
    std::string checksum;        // Checksum of file.
    Time created;                // Creation/modification time.
    Time valid;                  // Valid till time.
    Type type;                   // File type - usually file_type_file
  };

} // namespace Arc

#endif // __ARC_FILEINFO_H__
