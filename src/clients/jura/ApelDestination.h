#ifndef APELDESTINATION_H
#define APELDESTINATION_H

#include "Destination.h"
#include "JobLogFile.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc
{

  /** Reporting destination adapter for APEL. */
  class ApelDestination:public Destination
  {
  private:
    Arc::Logger logger;
    Arc::MCCConfig cfg;
    Arc::URL service_url;
    std::string topic;
    std::string output_dir;
    /** Max number of URs to put in a set before submitting it */
    int max_ur_set_size;
    /** Actual number of usage records in set */
    int urn;
    /** File name extension */
    int sequence;
    /** List of copies of job logs */
    std::list<JobLogFile> joblogs;
    /** Usage Record set map */
    std::vector< std::map<std::string,std::string> > usagerecordset_apel;
    
    int submit_batch();
    Arc::MCC_Status send_request(const std::string &urset);
    void clear();
    void XML2KeyValue(Arc::XMLNode &xml_urset, std::map<std::string, std::string> &key_urset);

  public:
    /** Constructor. Service URL and APEL-related parameters (e.g. UR
     *  batch size) are extracted from the given job log file.
     */
    ApelDestination(JobLogFile& joblog);
    /** Generates record from job log file content, collects it into the
     *  UR batch, and if batch is full, submits it to the service. 
     */
    void report(JobLogFile& joblog);
    void finish();
    ~ApelDestination();
  };

}

#endif
