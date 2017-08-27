#ifndef DESTINATION_H
#define DESTINATION_H


#include "JobLogFile.h"
#include "Config.h"
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>

namespace ArcJura
{
  
  /** Abstract class to represent a reporting destination.
   *  Specific destination types are represented by inherited classes. 
   */
  class Destination
  {
  public:
    /** Reports the job log file content to the destination. */
    virtual void report(JobLogFile &joblog)=0;
    /** Reports the archived job log file content to the destination. */
    virtual void report(std::string &joblog) {};
    /** Finishes pending submission of records. */
    virtual void finish() {};
    virtual ~Destination() {}

    /** Creates an instance of the inherited class corresponding to
     *  the destination for the given job log file. 
     */
    static Destination* createDestination(JobLogFile &joblog, const Config::ACCOUNTING &conf);
    std::string Current_Time( time_t parameter_time = time(NULL) );
    Arc::MCC_Status OutputFileGeneration(std::string prefix, Arc::URL url, std::string output_dir, std::string message,Arc::Logger& logger);
    /** Logged the sent jobIds */
    void log_sent_ids(Arc::XMLNode usagerecordset, int nr_of_records, Arc::Logger &logger,std::string type="");
  };
}

#endif
