#ifndef APELDESTINATION_H
#define APELDESTINATION_H

#include "CARAggregation.h"
#include "Destination.h"
#include "JobLogFile.h"
#include "Config.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <arc/XMLNode.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadSOAP.h>

namespace ArcJura
{

  /** Reporting destination adapter for APEL. */
  class ApelDestination:public Destination
  {
  private:
    Arc::Logger logger;
    Arc::MCCConfig cfg;
    const Config::APEL conf;
    Arc::URL service_url;
    std::string topic;
    std::string output_dir;
    /** Max number of URs to put in a set before submitting it */
    int max_ur_set_size;
    bool rereport;
    /** Require to set to ture this option by production message broker */
    bool use_ssl;
    /** Actual number of usage records in set */
    int urn;
    /** File name extension */
    int sequence;
    /** List of copies of job logs */
    std::list<JobLogFile> joblogs;
    /** Usage Record set XML */
    Arc::XMLNode usagerecordset;
    CARAggregation* aggregationManager;

    void init(std::string serviceurl_, std::string topic_, std::string outputdir_, std::string cert_, std::string key_, std::string ca_);
    int submit_batch();
    Arc::MCC_Status send_request(const std::string &urset);
    void clear();

  public:
    /** Constructor. Service URL and APEL-related parameters (e.g. UR
     *  batch size) are extracted from the given job log file.
     */
    ApelDestination(JobLogFile& joblog, const Config::APEL &conf);
    ApelDestination(std::string url_, std::string topic_);
    /** Generates record from job log file content, collects it into the
     *  UR batch, and if batch is full, submits it to the service. 
     */
    void report(JobLogFile& joblog);
    void report(std::string& joblog);
    void finish();
    ~ApelDestination();
  };

}

#endif
