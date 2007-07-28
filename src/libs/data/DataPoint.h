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
      std::list<URL> urls;         // Physical enpoints/URLs.
      unsigned long long int size; // Size of file in bytes.
      std::string checksum;        // Checksum of file.
      Time created;                // Creation/modification time.
      Time valid;                  // Valid till time.
      Type type;                   // File type - usually file_type_file
      FileInfo(const std::string& name = "") : name(name),
                                               size(-1),
                                               created(-1),
                                               valid(-1),
                                               type(file_type_unknown) {};
      /// If object is valid
      operator bool() {
        return (name.length() != 0);
      }
    };

    /// Constructor requires URL or meta-URL to be provided
    DataPoint(const URL& url);
    virtual ~DataPoint() {};

    /*
     *  META actions. Valid only for meta-URLs
     */

    /// Resolve meta-URL into list of ordinary URLs and obtain meta-information
    /// about file. Can be called for object representing ordinary URL or
    /// already resolved object.
    /// \param source true if DataPoint object represents source of information
    virtual bool meta_resolve(bool source __attribute__((unused))) {
      return false;
    };

    /// This function registers physical location of file into Indexing
    /// Service. It should be called *before* actual transfer to that
    /// location happens.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    /// \param force if true, perform registration of new file even if it
    /// already exists. Should be used to fix failures in Indexing Service.
    virtual bool meta_preregister(bool replication __attribute__((unused)),
                                  bool force __attribute__((unused)) = false) {
      return false;
    };

    /// Used for same purpose as meta_preregister. Should be called after
    /// actual transfer of file successfully finished.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    virtual bool meta_postregister(bool replication __attribute__((unused))) {
      return false;
    };

    // Same operation as meta_preregister and meta_postregister together.
    virtual bool meta_register(bool replication) {
      if(!meta_preregister(replication)) return false;
      if(!meta_postregister(replication)) return false;
      return true;
    };

    /// Should be called if file transfer failed. It removes changes made
    /// by meta_preregister.
    virtual bool meta_preunregister(bool replication __attribute__((unused))) {
      return false;
    };

    /// Remove information about file registered in Indexing Service.
    /// \param all if true information about file itself is (LFN) is removed.
    /// Otherwise only particular physical instance is unregistered.
    virtual bool meta_unregister(bool all __attribute__((unused))) {
      return false;
    };

    /// Obtain information about objects and their properties available
    /// under meta-URL of DataPoint object. It works only for meta-URL.
    /// \param files list of obtained objects.
    /// \param resolve if false, do not try to obtain propertiers of objects.
    virtual bool list_files(std::list<FileInfo>& files __attribute__((unused)),
                            bool resolve __attribute__((unused)) = true) {
      return false;
    };

    /// Retrieve properties of object pointed by meta-URL of DataPoint
    /// object. It works only for meta-URL.
    /// \param fi contains retrieved information.
    virtual bool get_info(FileInfo& fi __attribute__((unused))) {
      return false;
    };

    /*
     * Set and get corresponding meta-information related to URL.
     * Those attributes can be supported by non-meta-URLs too.
     */

    /// Check if meta-information 'size' is available.
    virtual bool meta_size_available() const {
      return (meta_size_ != (unsigned long long int)(-1));
    };

    /// Set value of meta-information 'size' if not already set.
    virtual void meta_size(unsigned long long int val) {
      if(!meta_size_available())
        meta_size_ = val;
    };

    /// Set value of meta-information 'size'.
    virtual void meta_size_force(unsigned long long int val) {
      meta_size_ = val;
    };

    /// Get value of meta-information 'size'.
    virtual unsigned long long int meta_size() const {
      return meta_size_;
    };

    /// Check if meta-information 'checksum' is available.
    virtual bool meta_checksum_available() const {
      return (!meta_checksum_.empty());
    };

    /// Set value of meta-information 'checksum' if not already set.
    virtual void meta_checksum(const std::string& val) {
      if(!meta_checksum_available())
        meta_checksum_ = val;
    };

    /// Set value of meta-information 'checksum'.
    virtual void meta_checksum_force(const std::string& val) {
      meta_checksum_ = val;
    };

    /// Get value of meta-information 'checksum'.
    virtual const std::string& meta_checksum() const {
      return meta_checksum_;
    };

    /// Check if meta-information 'creation/modification time' is available.
    virtual bool meta_created_available() const {
      return (meta_created_ != -1);
    };

    /// Set value of meta-information 'creation/modification time' if not
    /// already set.
    virtual void meta_created(Time val) {
      if(!meta_created_available())
        meta_created_ = val;
    };

    /// Set value of meta-information 'creation/modification time'.
    virtual void meta_created_force(Time val) {
      meta_created_ = val;
    };

    /// Get value of meta-information 'creation/modification time'.
    virtual Time meta_created() const {
      return meta_created_;
    };

    /// Check if meta-information 'validity time' is available.
    virtual bool meta_validtill_available() const {
      return (meta_validtill_ != -1);
    };

    /// Set value of meta-information 'validity time' if not already set.
    virtual void meta_validtill(Time val) {
      if(!meta_validtill_available())
        meta_validtill_ = val;
    };

    /// Set value of meta-information 'validity time'.
    virtual void meta_validtill_force(Time val) {
      meta_validtill_ = val;
    };

    /// Get value of meta-information 'validity time'.
    virtual Time meta_validtill() const {
      return meta_validtill_;
    };

    /// Check if URL is meta-URL.
    virtual bool meta() const {
      return false;
    };

    /// If endpoint can have any use from meta information.
    virtual bool accepts_meta() {
      return false;
    };

    /// If endpoint can provide at least some meta information directly.
    virtual bool provides_meta() {
      return false;
    };

    /// Acquire meta-information from another object. Defined values a
    /// not overwritten.
    /// \param p object from which information is taken.
    virtual void meta(const DataPoint& p) {
      if(p.meta_size_available())
        meta_size(p.meta_size());
      if(p.meta_checksum_available())
        meta_checksum(p.meta_checksum());
      if(p.meta_created_available())
        meta_created(p.meta_created());
      if(p.meta_validtill_available())
        meta_validtill(p.meta_validtill());
    };

    /// Compare meta-information form another object. Undefined values
    /// are not used for comparison. Default result is 'true'.
    /// \param p object to which compare.
    virtual bool meta_compare(const DataPoint& p) const {
      if(p.meta_size_available() && meta_size_available())
        if(meta_size_ != p.meta_size())
          return false;
      // TODO: compare checksums properly
      if(p.meta_checksum_available() && meta_checksum_available())
        if(strcasecmp(meta_checksum_.c_str(), p.meta_checksum().c_str()))
          return false;
      if(p.meta_created_available() && meta_created_available())
        if(meta_created_ != p.meta_created())
          return false;
      if(p.meta_validtill_available() && meta_validtill_available())
        if(meta_validtill_ != p.meta_validtill())
          return false;
      return true;
    };

    /// Check if file is registered in Indexing Service. Proper value is
    /// obtainable only after meta-resolve.
    virtual bool meta_stored() {
      return false;
    };

    /// Check if file is local (URL is something like file://).
    virtual bool local() const {
      return false;
    };

    virtual operator bool () const {
      return (bool)url;
    };

    virtual bool operator!() const {
      return !url;
    };

    /*
     *  Methods to manage list of locations.
     */

    /// Returns current (resolved) URL.
    virtual const URL& current_location() const {
      return empty_url_;
    };

    /// Returns meta information used to create curent URL. For RC that is
    ///  location's name. For RLS that is equal to pfn.
    virtual const std::string& current_meta_location() const {
      return empty_string_;
    };

    /// Switch to next location in list of URLs. At last location
    /// switch to first if number of allowed retries does not exceeded.
    /// Returns false if no retries left.
    virtual bool next_location() {
      return false;
    };

    /// Returns false if out of retries.
    virtual bool have_location() const {
      return false;
    };

    /// Returns true if number of resolved URLs is not 0.
    virtual bool have_locations() const {
      return false;
    };

    /// Remove current URL from list
    virtual bool remove_location() {
      return false;
    };

    /// Remove locations present in another DataPoint object
    virtual bool remove_locations(const DataPoint& p __attribute__((unused))) {
      return false;
    };

    /// Returns number of retries left.
    virtual int tries();

    /// Set number of retries.
    virtual void tries(int n);

    /// Returns URL which was passed to constructor
    virtual const URL& base_url() const;

    /// Add URL to list.
    /// \param meta meta-name (name of location/service).
    /// \param loc URL.
    virtual bool add_location(const std::string& meta __attribute__((unused)),
                              const URL& loc __attribute__((unused))) {
      return false;
    };

   protected:
    URL url;
    static Logger logger;
    static std::string empty_string_;
    static URL empty_url_;
    // attributes
    unsigned long long int meta_size_;
    std::string meta_checksum_;
    Time meta_created_;
    Time meta_validtill_;
    int tries_left;
  };

  /// DataPointIndex complements DataPoint with attributes
  /// common for meta-URLs
  /// It should never be used directly.
  class DataPointIndex : public DataPoint {
   protected:
    /// DataPointIndex::Location represents physical service at which
    /// files are located aka "base URL" inculding it's name (as given
    /// in Indexing Service).
    /// Currently it is used only internally by classes derived from
    /// DataPointIndex class and for printing debug information.
    class Location {
     public:
      std::string meta; // Given name of location
      URL url;          // location aka pfn aka access point
      bool existing;
      void *arg; // to be used by different pieces of soft differently
      Location() : existing(true), arg(NULL) {};
      Location(const URL& url) : url(url), existing(true), arg(NULL) {};
      Location(const std::string& meta, const URL& url,
               bool existing = true) : meta(meta), url(url),
                                       existing(existing), arg(NULL) {};
    };

    /// List of locations at which file can be probably found.
    std::list<Location> locations;
    std::list<Location>::iterator location;
   protected:
    bool is_metaexisting;
    bool is_resolved;
    void fix_unregistered(bool all);
   public:
    DataPointIndex(const URL& url);
    virtual ~DataPointIndex() {};
    virtual bool get_info(FileInfo& fi);

    virtual const URL& current_location() const {
      if(location != locations.end())
        return location->url;
      return empty_url_;
    };

    virtual const std::string& current_meta_location() const {
      if(location != locations.end())
        return location->meta;
      return empty_string_;
    };

    virtual bool next_location();
    virtual bool have_location() const;
    virtual bool have_locations() const;
    virtual bool remove_location();
    virtual bool remove_locations(const DataPoint& p);
    virtual bool add_location(const std::string& meta, const URL& loc);

    virtual bool meta() const {
      return true;
    };

    virtual bool accepts_meta() {
      return true;
    };

    virtual bool provides_meta() {
      return true;
    };

    virtual bool meta_stored() {
      return is_metaexisting;
    };

    virtual void tries(int n);
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
