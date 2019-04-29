#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include "Destination.h"
#include "LutsDestination.h"
#include "ApelDestination.h"
#include "CARDestination.h"

namespace ArcJura
{

  Destination* Destination::createDestination(JobLogFile &joblog, const Config::ACCOUNTING &conf)
  {
    std::string url=joblog["loggerurl"];
    if (url.substr(0,3) == "CAR") {
        return new CARDestination(joblog, (const Config::APEL &)conf);
    }
    //TODO distinguish
    if ( !joblog["topic"].empty() ||
         url.substr(0,4) == "APEL"){
        return new ApelDestination(joblog, (const Config::APEL &)conf);
    }else{
        return new LutsDestination(joblog, (const Config::SGAS &)conf);
    }
  }

  // Current time calculation and convert to the UTC time format.
  std::string Destination::Current_Time( time_t parameter_time ){

      time_t rawtime;
      if ( parameter_time == time(NULL) ){
          time ( &rawtime );    //current time
      } else {
          rawtime = parameter_time;
      }
      tm * ptm;
      ptm = gmtime ( &rawtime );

      std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
      std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
      std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
      std::string min_prefix = (ptm->tm_min < 10)?"0":"";
      std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
      std::stringstream out;
      if ( parameter_time == time(NULL) ){
          out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T"<<hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec<<"+0000";
      } else {
          out << ptm->tm_year+1900<<mon_prefix<<ptm->tm_mon+1<<day_prefix<<ptm->tm_mday<<"."<<hour_prefix<<ptm->tm_hour<<min_prefix<<ptm->tm_min<<sec_prefix<<ptm->tm_sec;
      }
      return out.str();
  }
  
  Arc::MCC_Status Destination::OutputFileGeneration(std::string prefix, Arc::URL url, std::string output_dir, std::string message, Arc::Logger& logger){
      //Filename generation
    int sequence = 0;
    std::string output_filename = prefix+"_records_" +url.Host() + "_" + Current_Time();
   logger.msg(Arc::DEBUG, 
               "UR set dump: %s",
               output_dir);
    if (!output_dir.empty()) {
        // local copy creation
        std::string output_path;
        output_path = output_dir;
        if (output_dir[output_dir.size()-1] != '/'){
            output_path = output_dir + "/";
        }
    
    
        output_path += output_filename;

        std::ifstream ifile(output_path.c_str());
        if (ifile) {
            // The file exists, and create new filename
            sequence++;
            std::stringstream ss;
            ss << sequence;
            output_path += ss.str();
            output_filename += ss.str();
        }
        else {
            sequence=0;
        }
        ifile.close();

        //Save all records into the output file.
        const char* filename(output_path.c_str());
        std::ofstream outputfile;
        outputfile.open (filename);
        if (outputfile.is_open())
        {
            outputfile << message;
            outputfile.close();
            logger.msg(Arc::DEBUG, "Backup file (%s) created.", output_filename);
        }
        else
        {
            return Arc::MCC_Status(Arc::PARSING_ERROR,
                   prefix + "client",
                   std::string(
                     "Error opening file: "
                               )+ 
                   filename
                   );
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  
  void Destination::log_sent_ids(Arc::XMLNode usagerecordset, int nr_of_records, Arc::Logger &logger, std::string type) {
        Arc::NS ns_query;
        std::string query = "";
        if ( type == "" ) {
            ns_query["urf"] = "http://schema.ogf.org/urf/2003/09/urf";
            query = "//JobUsageRecord/RecordIdentity";
        } else if ( type == "APEL" ) {
            ns_query["urf"] = "http://eu-emi.eu/namespaces/2012/11/computerecord";
            query = "//UsageRecord/RecordIdentity";
        }
        Arc::XMLNodeList list = usagerecordset.XPathLookup(query,ns_query);
        logger.msg(Arc::DEBUG, "Sent jobIDs: (nr. of job(s) %d)", nr_of_records);
        for (std::list<Arc::XMLNode>::iterator it = list.begin(); it != list.end(); it++) {
            std::string id = (*it).Attribute("urf:recordId");
            //std::size_t found = id.find_last_of("-");
            logger.msg(Arc::DEBUG, id);
        } 
  }
}

