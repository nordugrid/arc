#ifndef CARAGGREGATION_H
#define CARAGGREGATION_H

//#include "Destination.h"
//#include "JobLogFile.h"
//#include <stdexcept>
//#include <string>
//#include <vector>
//#include <map>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
//#include <arc/message/MCCLoader.h>
#include <arc/message/Message.h>
//#include <arc/message/PayloadSOAP.h>

namespace Arc
{

  /** Aggregation record collecting and reporting for APEL. */
  class CARAggregation
  {
  private:
    Arc::Logger logger;
    Arc::MCCConfig cfg;
    std::string host;
    std::string port;
    std::string topic;

    /** Require to set to ture this option by production message broker */
    std::string use_ssl;
    /** File name extension */
    int sequence;
    /** location of Aggregation Records */
    std::string aggr_record_location;
    bool aggr_record_update_need;
    /** Aggregation Record set XML */
    Arc::XMLNode aggregationrecordset;

    
    Arc::MCC_Status send_records(const std::string &urset);
    void clear();
    std::string Current_Time( time_t parameter_time = time(NULL) );
    ~CARAggregation();

  public:
    /**
     *  Constructor for information collection.
     */
    CARAggregation(std::string _host);
    /**
     *  Constructor for record reporting.
     */
    CARAggregation(std::string _host, std::string _port, std::string _topic);

    /** Generates record from CAR record, collects it into the
     *  CAR aggregation. 
     */
    void UpdateAggregationRecord(Arc::XMLNode& ur);
    
    /** Save the records to the disc.
     */
    int save_records();

  };

}

#endif
