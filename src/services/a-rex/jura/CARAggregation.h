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

    /** Require to set to true this option by production message broker */
    std::string use_ssl;
    /** File name extension */
    int sequence;
    /** location of Aggregation Records */
    std::string aggr_record_location;
    bool aggr_record_update_need;
    bool synch_message;
    /** Aggregation Record set XML */
    Arc::XMLNode aggregationrecordset;
    Arc::NS ns;

    void init(std::string _host, std::string _port, std::string _topic);
    /** Send records to the accounting server. */
    Arc::MCC_Status send_records(const std::string &urset);
    /** Update all records sending dates */
    void UpdateLastSendingDate();
    /** Update records sending dates that contains in the list */
    void UpdateLastSendingDate(Arc::XMLNodeList& records);
    void clear();
    std::string Current_Time( time_t parameter_time = time(NULL) );
    /** APEL Synch record generation from the CAR aggregation record */
    std::string SynchMessage(Arc::XMLNode records);

  public:
    /**
     *  Constructor for information collection.
     */
    CARAggregation(std::string _host);
    /**
     *  Constructor for record reporting.
     */
    CARAggregation(std::string _host, std::string _port, std::string _topic, bool synch);
    ~CARAggregation();

    /** Generated record from CAR record, collects it into the
     *  CAR aggregation. 
     */
    void UpdateAggregationRecord(Arc::XMLNode& ur);
    
    /** Save the records to the disc.
     */
    int save_records();

    /** Reporting a required record to the accounting server.
     */
    bool Reporting_records(std::string year, std::string month="");

    /** Reporting all records to the accounting server.
     */
    bool Reporting_records();
  };

}

#endif
