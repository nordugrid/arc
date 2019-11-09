#ifndef __ARC_CLEINT_COMPUTE_SUBMIT_COMMON_H_
#define __ARC_CLEINT_COMPUTE_SUBMIT_COMMON_H_

#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/OptionParser.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/JobInformationStorage.h>

#include "utils.h"

int process_submission_status(Arc::SubmissionStatus status, const Arc::UserConfig& usercfg);

void check_missing_plugins(Arc::Submitter s, int is_error);

int legacy_submit(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, std::list<Arc::Endpoint>& services, const std::string& requestedSubmissionInterface, const std::string& jobidfile, bool direct_submission);

int dumpjobdescription(const Arc::UserConfig& usercfg, const std::list<Arc::JobDescription>& jobdescriptionlist, const std::list<Arc::Endpoint>& services, const std::string& requestedSubmissionInterface);

/// Implements ARC6 logic of targets selection based on info/submit types requested
/**
  This helper method process requested types, computing elements and registry and 
  defines the endpoint batches for submission tries.
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] opt ClientOptions object containing request options
  \param endpoint_batches list of lists of Endpoint objects
  \return a bool indicating the need of target information lookup versus direct submission.
*/
bool prepare_submission_endpoint_batches(const Arc::UserConfig& usercfg, const ClientOptions& opt, std::list<std::list<Arc::Endpoint> >& endpoint_batches);

/// Submit job using defined endpoint batches and submission type
/**
  This helper method try to submit jobs to the list of endpoint batches 
  with (brokering) or without inforamtion quueries
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] endpoint_batches list of lists of Endpoint objects
  \param[in] info_discovery boolean indicating the need or inforamtion quueries and brokering
  \param[in] jobidoutfile Path to file to store jobids
  \param[in] jobdescriptionlist list of job descriptions to submit
  \return a bool indicating the need of target information lookup versus direct submission.
*/
int submit_jobs(const Arc::UserConfig& usercfg, const std::list<std::list<Arc::Endpoint> >& endpoint_batches, bool info_discovery, std::string jobidoutfile, const std::list<Arc::JobDescription>& jobdescriptionlist);

/// Class to handle submitted job and present the results to user
class HandleSubmittedJobs : public Arc::EntityConsumer<Arc::Job> {
public:
  HandleSubmittedJobs(const std::string& jobidfile, const Arc::UserConfig& uc) : jobidfile(jobidfile), uc(uc), submittedJobs() {}

  ~HandleSubmittedJobs() {}

  void addEntity(const Arc::Job& j);

  void write() const;

  void printsummary(const std::list<Arc::JobDescription>& originalDescriptions, const std::list<const Arc::JobDescription*>& notsubmitted) const;

  void clearsubmittedjobs() { submittedJobs.clear(); }

private:
  const std::string jobidfile;
  const Arc::UserConfig& uc;
  std::list<Arc::Job> submittedJobs;
};

#endif // __ARC_CLEINT_COMPUTE_SUBMIT_COMMON_H_
