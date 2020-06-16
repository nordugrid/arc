// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBINFORMATIONSTORAGE_H__
#define __ARC_JOBINFORMATIONSTORAGE_H__

#include <string>
#include <set>

#include <arc/compute/Job.h>

namespace Arc {
  
  class JobInformationStorage;
  
  typedef struct {
    const char *name;
    JobInformationStorage* (*instance)(const std::string&);
  } JobInformationStorageDescriptor;

  /// Abstract class for storing job information
  /**
   * This abstract class provides an interface which can be used to store job
   * information, which can then later be used to initialise Job objects from.
   * 
   * \note This class is abstract. All functionality is provided by specialised
   *  child classes.
   * 
   * \headerfile Job.h arc/compute/Job.h
   * \ingroup compute
   **/
  class JobInformationStorage {
  public:
    /// Constructor
    /**
     * Construct a JobInformationStorage object with name \c name. The name
     * could be a file name or maybe a database, that is implemention specific.
     * The \c nTries argument specifies the number times a lock on the storage
     * should be tried obtained for each method invocation. The constructor it
     * self should not acquire a lock through-out the object lifetime.
     * \c tryInterval is the waiting period in micro seconds between each
     * locking attemp.
     * 
     * @param name name of the storage.
     * @param nTries specifies the maximal number of times try to acquire a
     *  lock on storage to read from.
     * @param tryInterval specifies the interval (in micro seconds) between each
     *  attempt to acquire a lock.
     **/
    JobInformationStorage(const std::string& name, unsigned nTries = 10, unsigned tryInterval = 500000)
      : name(name), nTries(nTries), tryInterval(tryInterval), isValid(false), isStorageExisting(false) {}
    virtual ~JobInformationStorage() {}
    
    /// Check if storage is valid
    /**
     * @return true if storage is valid. 
     **/
    bool IsValid() const { return isValid; }
    
    /// Check if storage exists
    /**
     * @return true if storage already exist.
     **/
    bool IsStorageExisting() const { return isStorageExisting; }

    /// Read all jobs from storage
    /**
     * Read all jobs contained in storage, except those managed by a service at
     * an endpoint which matches any of those in the \c rejectEndpoints list
     * parameter. The read jobs are added to the list of Job objects referenced
     * by the \c jobs parameter. The algorithm used for matching should be
     * equivalent to that used in the URL::StringMatches method.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs is a reference to a list of Job objects, which will be filled
     *  with the jobs read from storage (cleared before use).
     * @param rejectEndpoints is a list of strings specifying endpoints for
     *  which Job objects with JobManagementURL matching any of those endpoints
     *  will not be part of the retrieved jobs. The algorithm used for matching
     *  should be equivalent to that used in the URL::StringMatches method.
     * @return \c true is returned if all jobs contained in the storage was
     *  retrieved (except those rejected, if any), otherwise false.
     **/
    virtual bool ReadAll(std::list<Job>& jobs, const std::list<std::string>& rejectEndpoints = std::list<std::string>()) = 0;
    
    /// Read specified jobs
    /**
     * Read jobs specified by job identifiers and/or endpoints from storage.
     * Only jobs which has a JobID or a Name attribute matching any of the items
     * in the \c identifiers list parameter, and also jobs for which the
     * \c JobManagementURL attribute matches any of those endpoints specified in
     * the \c endpoints list parameter, will be added to the
     * list of Job objects reference to by the \c jobs parameter, except those
     * jobs for which the \c JobManagementURL attribute matches any of those
     * endpoints specified in the \c rejectEndpoints list parameter. Identifiers
     * specified in the \c jobIdentifiers list parameter which matches a job in
     * the storage will be removed from the referenced list. The algorithm used
     * for matching should be equivalent to that used in the URL::StringMatches
     * method.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs reference to list of Job objects which will be filled with
     *  matching jobs.
     * @param jobIdentifiers specifies the job IDs and names of jobs to be added
     *  to the job list. Entries in this list is removed if they match a job
     *  from the storage.
     * @param endpoints is a list of strings specifying endpoints for
     *  which Job objects with the JobManagementURL attribute matching any of
     *  those endpoints will added to the job list. The algorithm used for
     *  matching should be equivalent to that used in the URL::StringMatches
     *  method.
     * @param rejectEndpoints is a list of strings specifying endpoints for
     *  which Job objects with the JobManagementURL attribute matching any of
     *  those endpoints will not be part of the retrieved jobs. The algorithm
     *  used for matching should be equivalent to that used in the
     *  URL::StringMatches method.
     * @return \c false is returned in case a job failed to be read from
     *  storage, otherwise \c true is returned. This method will also return in
     *  case an identifier does not match any jobs in the storage.
     **/
    virtual bool Read(std::list<Job>& jobs, std::list<std::string>& jobIdentifiers,
                      const std::list<std::string>& endpoints = std::list<std::string>(),
                      const std::list<std::string>& rejectEndpoints = std::list<std::string>()) = 0;

    /// Write jobs
    /**
     * Add jobs to storage. If there already exist a job with a specific job ID
     * in the storage, and a job with the same job ID is tried added to the
     * storage then the existing job will be overwritten. 
     *
     * A specialised implementaion does not necessarily need to be provided. If
     * not provided Write(const std::list<Job>&, std::set<std::string>&, std::list<const Job*>&)
     * will be used.
     *
     * @param jobs is the list of Job objects which should be added to the
     *  storage.
     * @return \c true is returned if all jobs in the \c jobs list are written
     *  to to storage, otherwise \c false is returned.
     * @see Write(const std::list<Job>&, std::set<std::string>&, std::list<const Job*>&)
     */
    virtual bool Write(const std::list<Job>& jobs)  { std::list<const Job*> newJobs; std::set<std::string> prunedServices; return Write(jobs, prunedServices, newJobs); }

    /// Write jobs
    /**
     * Add jobs to storage. If there already exist a job with a specific job ID
     * in the storage, and a job with the same job ID is tried added to the
     * storage then the existing job will be overwritten. For jobs in the
     * storage with a ServiceEndpointURL attribute where the host name is equal
     * to any of the entries in the set referenced by the \c prunedServices
     * parameter, is removed from the storage, if they are not among the list of
     * jobs referenced by the \c jobs parameter. A pointer to jobs in the job
     * list (\c jobs) which does not already exist in the storage will be added
     * to the list of Job object pointers referenced by the \c newJobs
     * parameter.
     * 
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobs is the list of Job objects which should be added to the
     *  storage.
     * @param prunedServices is a set of host names of services whose jobs
     *  should be removed if not replaced. This is typically the list of
     *  host names for which at least one endpoint was successfully queried.
     *  By passing an empty set, all existing jobs are kept, even if jobs are
     *  outdated.
     * @param newJobs is a reference to a list of pointers to Job objects which
     *  are not duplicates.
     * @return \c true is returned if all jobs in the \c jobs list are written
     *  to to storage, otherwise \c false is returned.
     **/
    virtual bool Write(const std::list<Job>& jobs, const std::set<std::string>& prunedServices, std::list<const Job*>& newJobs) = 0;

    /// Clean storage
    /**
     * Invoking this method causes the storage to be cleaned of any jobs it
     * holds.
     * 
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @return \c true is returned if the storage was successfully cleaned,
     *  otherwise \c false is returned.
     **/
    virtual bool Clean() = 0;

    /// Remove jobs
    /**
     * The jobs with matching job IDs (Job::JobID attribute) as specified with
     * the list of job IDs (\c jobids parameter) will be remove from the
     * storage.
     *
     * \note This method is abstract and an implementation must be provided by
     *  specialised classes.
     * 
     * @param jobids list job IDs for which matching jobs should be remove from
     *  storage. 
     * @return \c is returned if any of the matching jobs failed to be removed
     *  from the storage, otherwise \c true is returned.
     **/
    virtual bool Remove(const std::list<std::string>& jobids) = 0;
    
    /// Get name
    /**
     * @return Returns the name of the storage.
     **/
    const std::string& GetName() const { return name; }
    
    static JobInformationStorageDescriptor AVAILABLE_TYPES[];

  protected:
    const std::string name;
    unsigned nTries;
    unsigned tryInterval;
    /**
     * \since Added in 4.0.0.
     **/
    bool isValid;
    /**
     * \since Added in 4.0.0.
     **/
    bool isStorageExisting;
  };

} // namespace Arc

#endif // __ARC_JOBINFORMATIONSTORAGE_H__
