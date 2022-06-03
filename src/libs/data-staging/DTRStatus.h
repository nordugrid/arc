// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DTRSTATUS_H__
#define __ARC_DTRSTATUS_H__

#include <string>
#include <vector>

namespace DataStaging {
      
  /// Class representing the status of a DTR.
  /**
   * \ingroup datastaging
   * \headerfile DTRStatus.h arc/data-staging/DTRStatus.h
   */
  class DTRStatus {

  public:
    
    /// Possible state values
    enum DTRStatusType {
    	
      // ORDER IS IMPORTANT!!

      /// Just created
      NEW,

      /// Check the cache for the file may be already there 
      CHECK_CACHE,
      
      /// Checking the cache
      CHECKING_CACHE,

      /// Cache file is locked, waiting for its release
      CACHE_WAIT,
      
      /// Cache check completed
      CACHE_CHECKED,

      /// Resolve a meta-protocol
      RESOLVE,

      /// Resolving replicas
      RESOLVING,

      /// Replica resolution completed
      RESOLVED,

      /// Query a replica
      QUERY_REPLICA,

      /// Replica is being queried
      QUERYING_REPLICA,

      /// Replica was queried
      REPLICA_QUERIED,

      /// The destination should be deleted
      PRE_CLEAN,

      /// Deleting the destination
      PRE_CLEANING,

      /// The destination file has been deleted
      PRE_CLEANED,

      /// Prepare or stage the source and/or destination
      STAGE_PREPARE,

      /// Making a staging or preparing request
      STAGING_PREPARING,

      /// Wait for the status of the staging/preparing request
      STAGING_PREPARING_WAIT,

      /// Staging/preparing request completed
      STAGED_PREPARED,

      /// Transfer ready and can be started
      TRANSFER,

      /// Transfer is going
      TRANSFERRING,
      
      /// Transfer is on-going but scheduled for cancellation
      TRANSFERRING_CANCEL,

      /// Transfer completed
      TRANSFERRED,
      
      /// Transfer finished, release requests on the storage
      RELEASE_REQUEST,

      /// Releasing staging/preparing request
      RELEASING_REQUEST,

      /// Release of staging/preparing request completed
      REQUEST_RELEASED,
    
      /// Finalise replica (close catalog, send traces, etc)
      FINALISE_REPLICA,

      /// Finalising replica
      FINALISING_REPLICA,

      /// Replica finalised
      REPLICA_FINALISED,

      /// Register a new replica of the destination
      REGISTER_REPLICA,

      /// Registering a replica in an index service
      REGISTERING_REPLICA,
      
      /// Replica registration completed
      REPLICA_REGISTERED,
 
      /// Destination is cacheable, process cache
      PROCESS_CACHE,

      /// Releasing locks and copying/linking cache files to the session dir
      PROCESSING_CACHE,
      
      /// Cache processing completed
      CACHE_PROCESSED,

      /// Everything completed successfully
      DONE,

      /// Cancellation request fulfilled successfully
      CANCELLED,

      /// Cancellation request fulfilled but DTR also completed transfer successfully
      CANCELLED_FINISHED,

      /// Error occured
      ERROR,

      /// "Stateless" DTR
      NULL_STATE
      
    };
    
    /// Make new DTRStatus with given status and optional description.
    DTRStatus(const DTRStatusType& status, std::string desc="")
      : status(status), desc(desc) {}
    /// Make new DTRStatus with default NEW status
    DTRStatus() 
      : status(NEW), desc ("") {}

    /// Returns true if this status is the same as the given DTRStatusType
    bool operator==(const DTRStatusType& s) const {
      return status == s;
    }
    /// Returns true if this status is the same as the given DTRStatus
    bool operator==(const DTRStatus& s) const {
      return status == s.status;
    }
  

    /// Returns true if this status is not the same as the given DTRStatusType
    bool operator!=(const DTRStatusType& s) const {
      return status != s;
    }
    /// Returns true if this status is not the same as the given DTRStatus
    bool operator!=(const DTRStatus& s) const {
      return status != s.status;
    }
  
    /// Make a new DTRStatus with the same status as the given DTRStatusType
    DTRStatus& operator=(const DTRStatusType& s) {
      status = s;
      return *this;
    }

    /// Returns a string representation of the current state
    std::string str() const;

    /// Set the detailed description of the current state
    void SetDesc(const std::string& d) {
      desc = d;
    }
    
    /// Get the detailed description of the current state
    std::string GetDesc() const {
      return desc;
    }
    
    /// Get the DTRStatusType of the current state
    DTRStatusType GetStatus() const {
      return status;
    }

    // The actions in the following two vectors must match
    /// Vector of states with a to be processed action, eg CHECK_CACHE
    static const std::vector<DTRStatus::DTRStatusType> ToProcessStates;
    /// Vector of states with a processing action, eg CHECKING_CACHE
    static const std::vector<DTRStatus::DTRStatusType> ProcessingStates;

    /// Vector of states where a DTR is staged - used to limit the number of staged files
    static const std::vector<DTRStatus::DTRStatusType> StagedStates;

  private:
  
    /// status code
    DTRStatusType status;
    /// description set by the owner process
    std::string desc;

  }; // DTRStatus

  /// A class to represent error states reported by various components.
  /**
   * \ingroup datastaging
   * \headerfile DTRStatus.h arc/data-staging/DTRStatus.h
   */
  class DTRErrorStatus {

   public:

    /// A list of error types
    enum DTRErrorStatusType {

      /// No error
      NONE_ERROR,

      /// Internal error in Data Staging logic
      INTERNAL_LOGIC_ERROR,

      /// Internal processing error, like losing contact with external process
      INTERNAL_PROCESS_ERROR,

      /// Attempt to replicate a file to itself
      SELF_REPLICATION_ERROR,

      /// Permanent error with cache
      CACHE_ERROR,

      /// Temporary error with remote service
      TEMPORARY_REMOTE_ERROR,

      /// Permanent error with remote service
      PERMANENT_REMOTE_ERROR,

      /// Error with local file
      LOCAL_FILE_ERROR,

      /// Transfer rate was too slow
      TRANSFER_SPEED_ERROR,
      
      /// Waited for too long to become staging
      STAGING_TIMEOUT_ERROR
    };

    /// Describes where the error occurred
    enum DTRErrorLocation {

      /// No error
      NO_ERROR_LOCATION,

      /// Error with source
      ERROR_SOURCE,

      /// Error with destination
      ERROR_DESTINATION,

      /// Error during transfer not directly related to source or destination
      ERROR_TRANSFER,

      /// Error occurred in an unknown location
      ERROR_UNKNOWN
    };

    /// Create a new DTRErrorStatus with given error states
    /**
     * @param status Type of error
     * @param error_state DTR state in which the error occurred
     * @param location Location of error (at source, destination or during transfer)
     * @param desc Text description of error
     */
    DTRErrorStatus(DTRErrorStatusType status,
                   DTRStatus::DTRStatusType error_state,
                   DTRErrorLocation location,
                   const std::string& desc = ""):
      error_status(status),
      last_error_state(error_state),
      error_location(location),
      desc(desc) {};

    /// Create a new DTRErrorStatus with default none/null error states
    DTRErrorStatus() :
      error_status(NONE_ERROR),
      last_error_state(DTRStatus::NULL_STATE),
      error_location(NO_ERROR_LOCATION),
      desc("") {};

    /// Returns the error type
    DTRErrorStatusType GetErrorStatus() const {
     return error_status;
    }

    /// Returns the state in which the error occurred
    DTRStatus::DTRStatusType GetLastErrorState() const {
      return last_error_state.GetStatus();
    }

    /// Returns the location at which the error occurred
    DTRErrorLocation GetErrorLocation() const {
      return error_location;
    }

    /// Returns the error description
    std::string GetDesc() const {
      return desc;
    }

    /// Returns true if this error status is the same as the given DTRErrorStatusType
    bool operator==(const DTRErrorStatusType& s) const {
      return error_status == s;
    }
    /// Returns true if this error status is the same as the given DTRErrorStatus
    bool operator==(const DTRErrorStatus& s) const {
      return error_status == s.error_status;
    }

    /// Returns true if this error status is not the same as the given DTRErrorStatusType
    bool operator!=(const DTRErrorStatusType& s) const {
      return error_status != s;
    }
    /// Returns true if this error status is not the same as the given DTRErrorStatus
    bool operator!=(const DTRErrorStatus& s) const {
      return error_status != s.error_status;
    }

    /// Make a new DTRErrorStatus with the same error status as the given DTRErrorStatusType
    DTRErrorStatus& operator=(const DTRErrorStatusType& s) {
      error_status = s;
      return *this;
    }

   private:
    /// error state
    DTRErrorStatusType error_status;
    /// state that error occurred in
    DTRStatus last_error_state;
    /// place where the error occurred
    DTRErrorLocation error_location;
    /// description of error
    std::string desc;

  };

} // namespace DataStaging

#endif /*__ARC_DTRSTATUS_H_*/
