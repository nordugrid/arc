#include <string>
#include <map>

#include <unistd.h>

#include <glibmm.h>

namespace ARex {

class FileChunksList;

/// Representation of delivered file chunks
class FileChunks {
 friend class FileChunksList;
 private:
  Glib::Mutex lock;
  FileChunksList& list;
  std::map<std::string,FileChunks>::iterator self;
  typedef std::list<std::pair<off_t,off_t> > chunks_t;
  chunks_t chunks;
  off_t size;
  time_t last_accessed;
  int refcount;
  FileChunks(FileChunksList& container);
 public:
  FileChunks(const FileChunks& obj);
  /// Returns assigned file path (id of file)
  std::string Path(void) { return self->first; };
  /// Assign file size
  void Size(off_t size);
  /// Returns assigned file size
  off_t Size(void) { return size; };
  /// Report one more delivered chunk
  void Add(off_t start,off_t csize);
  /// Returns true if all chunks were delivered.
  bool Complete(void);
  /// Prints chunks delivered so far. For debuging purposes.
  void Print(void);
  /// Release reference obtained through FileChunksList::Get() method. 
  /// This operation may lead to destruction of FileChunk instance
  /// hence previously obtained refrence mus tnot be used.
  void Release(void);
  /// Relases reference obtained through Get() method and destroys its instance.
  /// Normally this method to be called instead of Release() after whole 
  /// file is delivered in order to free resources associated with 
  /// FileChunks instance.
  void Remove(void);
};

/// Container for FileChunks instances
class FileChunksList {
 friend class FileChunks;
 private:
  Glib::Mutex lock;
  typedef std::map<std::string,FileChunks> files_t;
  files_t files;
  int timeout;
  time_t last_timeout;
 public:
  FileChunksList(void);
  ~FileChunksList(void);
  /// Returns previously created FileChunks object with associated path.
  /// If such instance does not exist new one is created.
  /// Obtained reference may be used for other operations. 
  /// Obtained reference must be Release()ed after it is not longer needed.
  FileChunks& Get(std::string path);
  /// Assign timeout value (seconds) for file transfers
  void Timeout(int t) { timeout=t; };
  /// Returns pointer to first stuck file.
  /// File is considred stuck if its Add method was last called more
  /// timeout seconds ago.
  FileChunks* GetStuck(void);
  /// Returns pointer to first in a list created FileChunks instance.
  FileChunks* GetFirst(void);
};


} // namespace ARex

