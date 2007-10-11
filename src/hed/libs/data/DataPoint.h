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
    virtual bool CheckSize() const {
      return (size != (unsigned long long int)(-1));
    };

    /// Set value of meta-information 'size'.
    virtual void SetSize(const unsigned long long int val) {
      size = val;
    };

    /// Get value of meta-information 'size'.
    virtual unsigned long long int GetSize() const {
      return size;
    };

    /// Check if meta-information 'checksum' is available.
    virtual bool CheckCheckSum() const {
      return (!checksum.empty());
    };

    /// Set value of meta-information 'checksum'.
    virtual void SetCheckSum(const std::string& val) {
      checksum = val;
    };

    /// Get value of meta-information 'checksum'.
    virtual const std::string& GetCheckSum() const {
      return checksum;
    };

    /// Check if meta-information 'creation/modification time' is available.
    virtual bool CheckCreated() const {
      return (created != -1);
    };

    /// Set value of meta-information 'creation/modification time'.
    virtual void SetCreated(const Time& val) {
      created = val;
    };

    /// Get value of meta-information 'creation/modification time'.
    virtual const Time& GetCreated() const {
      return created;
    };

    /// Check if meta-information 'validity time' is available.
    virtual bool CheckValid() const {
      return (valid != -1);
    };

    /// Set value of meta-information 'validity time'.
    virtual void SetValid(const Time& val) {
      valid = val;
    };

    /// Get value of meta-information 'validity time'.
    virtual const Time& GetValid() const {
      return valid;
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

    /// Acquire meta-information from another object. Defined values are
    /// not overwritten.
    /// \param p object from which information is taken.
    virtual void meta(const DataPoint& p) {
      if(!CheckSize())
        SetSize(p.GetSize());
      if(!CheckCheckSum())
        SetCheckSum(p.GetCheckSum());
      if(!CheckCreated())
        SetCreated(p.GetCreated());
      if(!CheckValid())
        SetValid(p.GetValid());
    };

    /// Compare meta-information form another object. Undefined values
    /// are not used for comparison. Default result is 'true'.
    /// \param p object to which compare.
    virtual bool meta_compare(const DataPoint& p) const {
      if(CheckSize() && p.CheckSize())
        if(GetSize() != p.GetSize())
          return false;
      // TODO: compare checksums properly
      if(CheckCheckSum() && p.CheckCheckSum())
        if(strcasecmp(GetCheckSum().c_str(), p.GetCheckSum().c_str()))
          return false;
      if(CheckCreated() && p.CheckCreated())
        if(GetCreated() != p.GetCreated())
          return false;
      if(CheckValid() && p.CheckValid())
        if(GetValid() != p.GetValid())
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
    unsigned long long int size;
    std::string checksum;
    Time created;
    Time valid;
    int tries_left;
  };

} // namespace Arc

#endif // __ARC_DATAPOINT_H__
