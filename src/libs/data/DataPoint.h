#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <string>
#include <list>
#include <map>

#include "common/DateTime.h"
#include "common/URL.h"

namespace Arc {

class Logger;

/// DataPoint is an abstraction of URL. It can handle URLs of type file://, 
/// ftp://, gsiftp://, http://, https://, httpg:// (HTTP over GSI), 
/// se:// (NG web service over HTTPG) and meta-URLs (URLs of Infexing 
/// Services) rc://, rls://.
/// DataPoint provides means to resolve meta-URL into multiple URLs and
/// to loop through them.


class DataPoint {
 public:
  /// FileInfo stores information about file (meta-information). Although all
  /// members are public it is mot desirable to modify them directly outside
  /// DataPoint class.
  class FileInfo {
   public:
    typedef enum {
      file_type_unknown = 0,
      file_type_file = 1,
      file_type_dir = 2
    } Type;
    std::string name;
    std::list<URL> urls;          /// Physical enpoints/URLs at which file can be accessed.
    unsigned long long int size;  /// Size of file in bytes.
    bool size_available;          /// If size is known.
    std::string checksum;  /// Checksum of file.
    bool checksum_available;          /// If checksum is known.
    Time created;            /// Creation/modification time.
    bool created_available;  /// If time is known.
    Time valid;              /// Valid till time.
    bool valid_available;    /// If validity is known.
    Type type;               /// File type - usually file_type_file
    FileInfo(const std::string& name = ""):name(name),size_available(false),checksum_available(false),created_available(false),valid_available(false),type(file_type_unknown) { };
    operator bool(void) { return (name.length() != 0); }; /// If object is valid
  };

  /// Constructor requires URL or meta-URL to be provided
  DataPoint(const URL& url);
  virtual ~DataPoint(void) {};

  /* 
   *  META actions. Valid only for meta-URLs
   */
  /// Resolve meta-URL into list of ordinary URLs and obtain meta-information
  /// about file. Can be called for object representing ordinary URL or
  /// already resolved object.
  /// \param source true if DataPoint object represents source of information
  virtual bool meta_resolve(bool source) { return false; };
  ///  This function registers physical location of file into Indexing
  ///  Service. It should be called *before* actual transfer to that
  ///  location happens.
  ///  \param replication if true then file is being replicated between 2 locations registered in Indexing Service under same name.
  ///  \param force if true, perform registration of new file even if it already exists. Should be used to fix failures in Indexing Service.
  virtual bool meta_preregister(bool replication,bool force = false) { return false; };
  /// Used for same purpose as meta_preregister. Should be called after
  /// actual transfer of file successfully finished.
  ///  \param replication if true then file is being replicated between 2 locations registered in Indexing Service under same name.
  /// \param failure not used.
  virtual bool meta_postregister(bool replication,bool failure) { return false; };
  // Same operation as meta_preregister and meta_postregister together.
  virtual bool meta_register(bool replication) {
    if(!meta_preregister(replication)) return false;
    if(!meta_postregister(replication,false)) return false;
    return true;
  };
  ///  Should be called if file transfer failed. It removes changes made
  ///  by meta_preregister.
  virtual bool meta_preunregister(bool replication) { return false; };
  ///  Remove information about file registered in Indexing Service.
  ///  \param all if true information about file itself is (LFN) is removed. Otherwise only particular physical instance is unregistered.
  virtual bool meta_unregister(bool all) { return false; };
  /// Obtain information about objects and their properties available
  /// under meta-URL of DataPoint object. It works only for meta-URL.
  /// \param files list of obtained objects.
  /// \param resolve if false, do not try to obtain propertiers of objects.
  virtual bool list_files(std::list<DataPoint::FileInfo> &files,bool resolve = true) { return false; };
  /// Retrieve properties of object pointed by meta-URL of DataPoint
  /// object. It works only for meta-URL.
  /// \param fi contains retrieved information.
  virtual bool get_info(DataPoint::FileInfo &fi) { return false; };
  /*
   * Set and get corresponding meta-information related to URL.
   * Those attributes can be supported by non-meta-URLs too.  
   */
  /// Check if meta-information 'size' is available.
  virtual bool meta_size_available(void) const { return meta_size_valid; };
  /// Set value of meta-information 'size' if not already set.
  virtual void meta_size(unsigned long long int val) {
    if(!meta_size_valid) { meta_size_=val; meta_size_valid=true; };
  };
  /// Set value of meta-information 'size'.
  virtual void meta_size_force(unsigned long long int val) {
    meta_size_=val; meta_size_valid=true;
  };
  /// Get value of meta-information 'size'.
  virtual unsigned long long int meta_size(void) const {
    if(meta_size_valid) return meta_size_; return 0;
  };
  /// Check if meta-information 'checksum' is available.
  virtual bool meta_checksum_available(void) const { return meta_checksum_valid; };
  /// Set value of meta-information 'checksum' if not already set.
  virtual void meta_checksum(const std::string& val) {
    if(!meta_checksum_valid) { meta_checksum_=val; meta_checksum_valid=true; };
  };
  /// Set value of meta-information 'checksum'.
  virtual void meta_checksum_force(const std::string& val) {
    meta_checksum_=val; meta_checksum_valid=true;
  };
  /// Get value of meta-information 'checksum'.
  virtual const std::string& meta_checksum(void) const {
    if(meta_checksum_valid) return meta_checksum_;
  };
  /// Check if meta-information 'creation/modification time' is available.
  virtual bool meta_created_available(void) const { return meta_created_valid; };
  /// Set value of meta-information 'creation/modification time' if not already set.
  virtual void meta_created(Time val) {
    if(!meta_created_valid) { meta_created_=val; meta_created_valid=true; };
  };
  /// Set value of meta-information 'creation/modification time'.
  virtual void meta_created_force(Time val) {
    meta_created_=val; meta_created_valid=true;
  };
  /// Get value of meta-information 'creation/modification time'.
  virtual Time meta_created(void) const {
    if(meta_created_valid) return meta_created_; return 0;
  };
  /// Check if meta-information 'validity time' is available.
  virtual bool meta_validtill_available(void) const { return meta_validtill_valid; };
  /// Set value of meta-information 'validity time' if not already set.
  virtual void meta_validtill(Time val) {
    if(!meta_validtill_valid) {meta_validtill_=val; meta_validtill_valid=true;};  };
  /// Set value of meta-information 'validity time'.
  virtual void meta_validtill_force(Time val) {
    meta_validtill_=val; meta_validtill_valid=true;
  };
  /// Get value of meta-information 'validity time'.
  virtual Time meta_validtill(void) const {
    if(meta_validtill_valid) return meta_validtill_; return 0;
  };
  /// Check if URL is meta-URL.
  virtual bool meta(void) const  { return false; };
  /// If endpoint can have any use from meta information.
  virtual bool accepts_meta(void) { return false; };
   /// If endpoint can provide at least some meta information directly.
  virtual bool provides_meta(void) { return false; };
  /// Acquire meta-information from another object. Defined values a
  /// not overwritten.
  /// \param p object from which information is taken.
  virtual void meta(const DataPoint &p) {
    if(p.meta_size_available())      meta_size(p.meta_size());
    if(p.meta_checksum_available())  meta_checksum(p.meta_checksum());
    if(p.meta_created_available())   meta_created(p.meta_created());
    if(p.meta_validtill_available()) meta_validtill(p.meta_validtill());
  };
   /// Compare meta-information form another object. Undefined values
  /// are not used for comparison. Default result is 'true'.
  /// \param p object to which compare.
  virtual bool meta_compare(const DataPoint &p) const {
    if(p.meta_size_available() && meta_size_valid)
      if(meta_size_ != p.meta_size()) return false;
    // TODO: compare checksums properly
    if(p.meta_checksum_available() && meta_checksum_valid)
      if(strcasecmp(meta_checksum_.c_str(),p.meta_checksum().c_str())) return false;
    if(p.meta_created_available() && meta_created_valid)
      if(meta_created_ != p.meta_created()) return false;
    if(p.meta_validtill_available() && meta_validtill_valid)
      if(meta_validtill_ != p.meta_validtill()) return false;
    return true;
  };
  /// Check if file is registered in Indexing Service. Proper value is
  /// obtainable only after meta-resolve.
  virtual bool meta_stored(void) { return false; };
  /// Check if file is local (URL is something like file://).
  virtual bool local(void) const { return false; };
  virtual operator bool (void) const { return (bool)url; };
  virtual bool operator !(void) const { return !url; };
  /*
   *  Methods to manage list of locations.
   */
  /// Returns current (resolved) URL.
  virtual const URL& current_location(void) const {};
  /// Returns meta information used to create curent URL. For RC that is
  ///  location's name. For RLS that is equal to pfn.
  virtual const std::string& current_meta_location(void) const {};
  /// Switch to next location in list of URLs. At last location
  /// switch to first if number of allowed retries does not exceeded.
  /// Returns false if no retries left.
  virtual bool next_location(void) { return false; };
  /// Returns false if out of retries.
  virtual bool have_location(void) const { return false; };
  /// Returns true if number of resolved URLs is not 0.
  virtual bool have_locations(void) const { return false; };
  /// Remove current URL from list
  virtual bool remove_location(void) { return false; };
  /// Remove locations present in another DataPoint object
  virtual bool remove_locations(const DataPoint& p) { return false; };
  /// Returns number of retries left.
  virtual int tries(void);
  /// Set number of retries.
  virtual void tries(int n);
  /// Returns URL which was passed to constructor
  virtual const URL& base_url(void) const;
  /// Add URL to list.
  /// \param meta meta-name (name of location/service).
  /// \param loc URL.
  virtual bool add_location(const std::string& meta,const URL& loc) { return false; };

 protected:
  URL url;
  static Logger logger;
  int tries_left;
  // attributes
  unsigned long long int meta_size_;
  bool meta_size_valid;
  std::string meta_checksum_;
  bool meta_checksum_valid;
  Time meta_created_;
  bool meta_created_valid;
  Time meta_validtill_;
  bool meta_validtill_valid;
};

/// DataPointIndex complements DataPoint with attributes
/// common for meta-URLs
/// It should never be used directly.
class DataPointIndex : public DataPoint {
 protected:
  /// DataPointIndex::Location represents physical service at which files are
  /// located aka "base URL" inculding it's name (as given in Indexing Service).
  /// Currently it is used only internally by classes derived from 
  /// DataPointIndex class and for printing debug information.
  class Location {
   public:
    std::string meta; // Given name of location
    URL url;          // location aka pfn aka access point
    bool existing;
    void* arg;   // to be used by different pieces of soft differently
    Location(void):existing(true),arg(NULL) { };
    Location(const URL& url):url(url),existing(true),arg(NULL) { };
    Location(const std::string& meta,const URL& url,bool existing=true):meta(meta),url(url),existing(existing),arg(NULL) { };
  };

  std::list<Location> locations; /// List of locations at which file can be probably found.
  std::list<Location>::iterator location;
 protected:
  bool is_metaexisting;
  bool is_resolved;
  void fix_unregistered(bool all);
 public:
  DataPointIndex(const URL& url);
  virtual ~DataPointIndex(void) { };
  virtual bool get_info(DataPoint::FileInfo &fi);
  virtual const URL& current_location(void) const {
    if(location != locations.end())
      return location->url;
  };
  virtual const std::string& current_meta_location(void) const {
    if(location != locations.end())
      return location->meta;
  };
  virtual bool next_location(void);
  virtual bool have_location(void) const;
  virtual bool have_locations(void) const;
  virtual bool remove_location(void);
  virtual bool remove_locations(const DataPoint& p);
  virtual bool add_location(const std::string& meta,const URL& loc);
  virtual bool meta(void) const { return true; };
  virtual bool accepts_meta(void) { return true; };
  virtual bool provides_meta(void) { return true; };
  virtual bool meta_stored(void) { return is_metaexisting; };
  virtual void tries(int n);
};

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
