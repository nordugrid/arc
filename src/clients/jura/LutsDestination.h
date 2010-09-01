#ifndef LUTSDESTINATION_H
#define LUTSDESTINATION_H

#include "Destination.h"
#include "JobLogFile.h"
#include <stdexcept>
#include <string>
#include <map>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc
{

  /** Reporting destination adapter for SGAS LUTS. */
  class LutsDestination:public Destination
  {
  private:
    Arc::Logger logger;
    Arc::MCCConfig cfg;
    Arc::URL service_url;
    /** Max number of URs to put in a set before submitting it */
    int max_ur_set_size;
    /** Actual number of usage records in set */
    int urn;
    /** List of copies of job logs */
    std::list<JobLogFile> joblogs;
    /** Usage Record set XML */
    Arc::XMLNode usagerecordset;
    
    int submit_batch();
    Arc::MCC_Status send_request(const std::string &urset);
    void clear();

  public:
    /** Constructor. Service URL and LUTS-related parameters (e.g. UR
     *  batch size) are extracted from the given job log file.
     */
    LutsDestination(JobLogFile& joblog);
    /** Generates record from job log file content, collects it into the
     *  UR batch, and if batch is full, submits it to the service. 
     */
    void report(JobLogFile& joblog);
    void finish();
    ~LutsDestination();
  };

}

#endif
