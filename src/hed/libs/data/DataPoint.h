#ifndef __ARC_DATAPOINT_H__
#define __ARC_DATAPOINT_H__

#include <string>
#include <list>
#include <map>

#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/data/FileInfo.h>

namespace Arc {

  class Logger;

  /// This class is an abstraction of URL. 
  /** It can handle URLs of type file://, ftp://, gsiftp://, http://, https://,
   httpg:// (HTTP over GSI), se:// (NG web service over HTTPG) and meta-URLs 
   (URLs of Infexing Services) rc://, rls://.
   DataPoint provides means to resolve meta-URL into multiple URLs and
   to loop through them. */

  class DataPoint {
   public:
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
    virtual bool meta_resolve(bool) {
      return false;
    };

    /// This function registers physical location of file into Indexing
    /// Service. It should be called *before* actual transfer to that
    /// location happens.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    /// \param force if true, perform registration of new file even if it
    /// already exists. Should be used to fix failures in Indexing Service.
    virtual bool meta_preregister(bool, bool = false) {
      return false;
    };

    /// Used for same purpose as meta_preregister. Should be called after
    /// actual transfer of file successfully finished.
    /// \param replication if true then file is being replicated between
    /// 2 locations registered in Indexing Service under same name.
    virtual bool meta_postregister(bool) {
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
    virtual bool meta_preunregister(bool) {
      return false;
    };

    /// Remove information about file registered in Indexing Service.
    /// \param all if true information about file itself is (LFN) is removed.
    /// Otherwise only particular physical instance is unregistered.
    virtual bool meta_unregister(bool) {
      return false;
    };

    /// Obtain information about objects and their properties available
    /// under meta-URL of DataPoint object. It works only for meta-URL.
    /// \param files list of obtained objects.
    /// \param resolve if false, do not try to obtain propertiers of objects.
    virtual bool list_files(std::list<FileInfo>&, bool = true) {
      return false;
    };

    /// Retrieve properties of object pointed by meta-URL of DataPoint
    /// object. It works only for meta-URL.
    /// \param fi contains retrieved information.
    virtual bool get_info(FileInfo&) {
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
    virtual bool remove_locations(const DataPoint&) {
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
    virtual bool add_location(const std::string&, const URL&) {
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

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
