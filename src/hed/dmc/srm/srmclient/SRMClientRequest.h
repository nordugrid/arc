#ifndef SRMCLIENTREQUEST_H_
#define SRMCLIENTREQUEST_H_

#include <arc/Logger.h>

namespace ArcDMCSRM {

  /// The version of the SRM protocol
  enum SRMVersion {
    SRM_V1,
    SRM_V2_2,
    SRM_VNULL
  };

  /// Specifies whether file is on disk or only on tape
  enum SRMFileLocality {
    SRM_ONLINE,
    SRM_NEARLINE,
    SRM_UNKNOWN,
    SRM_STAGE_ERROR
  };

  /// Quality of retention
  enum SRMRetentionPolicy {
    SRM_REPLICA,
    SRM_OUTPUT,
    SRM_CUSTODIAL,
    SRM_RETENTION_UNKNOWN
  };

  /// The lifetime of the file
  enum SRMFileStorageType {
    SRM_VOLATILE,
    SRM_DURABLE,
    SRM_PERMANENT,
    SRM_FILE_STORAGE_UNKNOWN
  };

  /// File, directory or link
  enum SRMFileType {
    SRM_FILE,
    SRM_DIRECTORY,
    SRM_LINK,
    SRM_FILE_TYPE_UNKNOWN
  };

  /// Implementation of service. Found from srmPing (v2.2 only)
  enum SRMImplementation {
    SRM_IMPLEMENTATION_DCACHE,
    SRM_IMPLEMENTATION_CASTOR,
    SRM_IMPLEMENTATION_DPM,
    SRM_IMPLEMENTATION_STORM,
    SRM_IMPLEMENTATION_UNKNOWN
  };

  /// General exception to represent a bad SRM request
  class SRMInvalidRequestException
    : public std::exception {};

  /// The status of a request
  enum SRMRequestStatus {
    SRM_REQUEST_CREATED,
    SRM_REQUEST_ONGOING,
    SRM_REQUEST_FINISHED_SUCCESS,
    SRM_REQUEST_FINISHED_PARTIAL_SUCCESS,
    SRM_REQUEST_FINISHED_ERROR,
    SRM_REQUEST_SHOULD_ABORT,
    SRM_REQUEST_CANCELLED
  };

  /// Class to represent a SRM request.
  /**
   * It may be used for multiple operations, for example calling getTURLs()
   * sets the request token in the request object (for a v2.2 client) and
   * then same object is passed to releaseGet().
   */
  class SRMClientRequest {

  public:
    /// Creates a request object with multiple SURLs.
    /**
     * The URLs here are in the form
     * srm://srm.host.org/path/to/file
     */
    SRMClientRequest(const std::list<std::string>& urls)
        : _request_id(0),
          _space_token(""),
          _waiting_time(1),
          _status(SRM_REQUEST_CREATED),
          _request_timeout(60),
          _total_size(0),
          _long_list(false),
          _recursion(0),
          _offset(0),
          _count(0) {
      if (urls.empty())
        throw SRMInvalidRequestException();
      for (std::list<std::string>::const_iterator it = urls.begin();
           it != urls.end(); ++it)
        _surls[*it] = SRM_UNKNOWN;
    };

    /// Creates a request object with a single SURL.
    /**
     * The URL here is in the form
     * srm://srm.host.org/path/to/file
     */
    SRMClientRequest(const std::string& url="", const std::string& id="")
        : _request_id(0),
          _space_token(""),
          _waiting_time(1),
          _status(SRM_REQUEST_CREATED),
          _request_timeout(60),
          _total_size(0),
          _long_list(false),
          _recursion(0),
          _offset(0),
          _count(0) {
      if (url.empty() && id.empty())
        throw SRMInvalidRequestException();
      if (!url.empty())
        _surls[url] = SRM_UNKNOWN;
      else
        _request_token = id;
    }

    void request_id(int id) { _request_id = id; }
    int request_id() const { return _request_id; }

    void request_token(const std::string& token) { _request_token = token; }
    std::string request_token() const { return _request_token; }

    void file_ids(const std::list<int>& ids) { _file_ids = ids; }
    std::list<int> file_ids() const { return _file_ids; }

    void space_token(const std::string& token) { _space_token = token; }
    std::string space_token() const { return _space_token; }

    /// Returns the first surl in the list
    std::string surl() const { return _surls.begin()->first; }

    std::list<std::string> surls() const {
      std::list<std::string> surl_list;
      for (std::map<std::string, SRMFileLocality>::const_iterator it =
             _surls.begin(); it != _surls.end(); ++it) {
        surl_list.push_back(it->first);
      }
      return surl_list;
    }

    void surl_statuses(const std::string& surl, SRMFileLocality locality) { _surls[surl] = locality; }
    std::map<std::string, SRMFileLocality> surl_statuses() const { return _surls; }

    void surl_failures(const std::string& surl, const std::string& reason) { _surl_failures[surl] = reason; }
    std::map<std::string, std::string> surl_failures() const { return _surl_failures; }

    void waiting_time(int wait_time) { _waiting_time = wait_time; }
    /// Get waiting time. A waiting time of zero means no estimate was given
    /// by the remote service.
    int waiting_time() const { return _waiting_time; }

    /// Set status to SRM_REQUEST_FINISHED_SUCCESS
    void finished_success() { _status = SRM_REQUEST_FINISHED_SUCCESS; }
    /// Set status to SRM_REQUEST_FINISHED_PARTIAL_SUCCESS
    void finished_partial_success() { _status = SRM_REQUEST_FINISHED_PARTIAL_SUCCESS; }
    /// Set status to SRM_REQUEST_FINISHED_ERROR
    void finished_error() { _status = SRM_REQUEST_FINISHED_ERROR; }
    /// Set status to SRM_REQUEST_SHOULD_ABORT
    void finished_abort() { _status = SRM_REQUEST_SHOULD_ABORT; }
    /// Set waiting time to t and status to SRM_REQUEST_ONGOING
    void wait(int t = 0) {
      _status = SRM_REQUEST_ONGOING;
      _waiting_time = t;
    }
    /// Set status to SRM_REQUEST_CANCELLED
    void cancelled() { _status = SRM_REQUEST_CANCELLED; }
    /// Get status
    SRMRequestStatus status() const { return _status; }

    void request_timeout(unsigned int timeout) { _request_timeout = timeout; };
    unsigned int request_timeout() const { return _request_timeout; };

    void total_size(unsigned long long size) { _total_size = size; };
    unsigned long long total_size() const { return _total_size; };

    void long_list(bool list) { _long_list = list; }
    bool long_list() const { return _long_list; }

    void transport_protocols(const std::list<std::string>& protocols) { _transport_protocols = protocols; }
    std::list<std::string> transport_protocols() const { return _transport_protocols; }

    void recursion(int level) { _recursion = level; }
    int recursion() const { return _recursion; }

    void offset(unsigned int no) { _offset = no; }
    unsigned int offset() const { return _offset; }

    void count(unsigned int no) { _count = no; }
    unsigned int count() const { return _count; }

  private:

    /// The SURLs of the files involved in the request, mapped to their locality.
    std::map<std::string, SRMFileLocality> _surls;

    /// int ids are used in SRM1
    int _request_id;

    /// string request tokens (eg "-21249586") are used in SRM2.2
    std::string _request_token;

    /// A list of file ids is kept in SRM1
    std::list<int> _file_ids;

    /// The space token associated with a request
    std::string _space_token;

    /// A map of SURLs for which requests failed to failure reason.
    /// Used for bring online requests.
    std::map<std::string, std::string> _surl_failures;

    /// Estimated waiting time as returned by the server to wait
    /// until the next poll of an asychronous request.
    int _waiting_time;

    /// Status of request. Only useful for asynchronous requests.
    SRMRequestStatus _status;

    /**
     * For operations like getTURLs and putTURLs _request_timeout specifies
     * the timeout for these operations to complete. If it is zero then
     * these operations will act asynchronously, i.e. return and expect
     * the caller to poll for the status. If it is non-zero the operations
     * will block until completed or this timeout has been reached.
     */
    unsigned int _request_timeout;

    /// Total size of all files in request. Can be used when reserving space.
    unsigned long long _total_size;

    /// Whether a detailed listing is requested
    bool _long_list;

    /// List of requested transport protocols
    std::list<std::string> _transport_protocols;

    /// Recursion level (for list or stat requests only)
    int _recursion;

    /// Offset at which to start listing (for large directories)
    unsigned int _offset;

    /// How many files to list, used with _offset
    unsigned int _count;

  };

} // namespace ArcDMCSRM

#endif /* SRMCLIENTREQUEST_H_ */
