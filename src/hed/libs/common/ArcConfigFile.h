#ifndef __ARC_CONFIG_FILE_H__
#define __ARC_CONFIG_FILE_H__

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace Arc {

class ConfigFile: public std::ifstream {
 public:

  /// Default constructor creates object not associated with file.
  ConfigFile(void) { };

  /// Constructor creates object associated with file located at path.
  ConfigFile(const std::string &path) { open(path); };

  /// Open/assign configuration file located at path to this object.
  bool open(const std::string &path);

  /// Closes configuration file.
  bool close(void);

  /// Read one line of configuration file.
  /// Returns string containing whole line.
  std::string read_line();

  /// Helper function to read one line of configuration from provided stream.
  static std::string read_line(std::istream& stream);

  /// Recognizable configuration file types.
  typedef enum {
    file_XML,
    file_INI,
    file_unknown
  } file_type;

  /// Detect type of currently associated configuration file.
  file_type detect(void);

};

} // namespace Arc

#endif // __ARC_CONFIG_FILE_H__

