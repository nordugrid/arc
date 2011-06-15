#include "ApelDestination.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"
#include <arc/Utils.h>
#include <arc/client/ClientInterface.h>
#include <sstream>

namespace Arc
{
  // Current time calculation and convert to the UTC time format.
  std::string Current_Time( time_t parameter_time = time(NULL) ){

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

  ApelDestination::ApelDestination(JobLogFile& joblog):
    logger(Arc::Logger::rootLogger, "JURA.ApelDestination"),
    urn(0),
    sequence(0)
  {
    //Get service URL, cert, key, CA path from job log file
    std::string serviceurl=joblog["loggerurl"];
    topic==joblog["topic"];
    output_dir=joblog["outputdir"];
    std::string certfile=joblog["certificate_path"];
    std::string keyfile=joblog["key_path"];
    std::string cadir=joblog["ca_certificates_dir"];
    // ...or get them from environment
    if (certfile.empty())
      certfile=Arc::GetEnv("X509_USER_CERT");
    if (keyfile.empty())
      keyfile=Arc::GetEnv("X509_USER_KEY");
    if (cadir.empty())
      cadir=Arc::GetEnv("X509_CERT_DIR");
    // ...or by default, use host cert, key, CA path
    if (certfile.empty())
      certfile=JURA_DEFAULT_CERT_FILE;
    if (keyfile.empty())
      keyfile=JURA_DEFAULT_KEY_FILE;
    if (cadir.empty())
      cadir=JURA_DEFAULT_CA_DIR;

    cfg.AddCertificate(certfile);
    cfg.AddPrivateKey(keyfile);
    cfg.AddCADir(cadir);

    //  Tokenize service URL
    std::string host, port, endpoint;
    if (serviceurl.empty())
      {
        logger.msg(Arc::ERROR, "ServiceURL missing");
      }
    else
      {
        Arc::URL url(serviceurl);
        service_url = url;
        if (url.Protocol()!="https")
          {
            logger.msg(Arc::ERROR, "Protocol is %s, should be https",
                       url.Protocol());
          }
        host=url.Host();
        std::ostringstream os;
        os<<url.Port();
        port=os.str();
        endpoint=url.Path();
      }

    //Get Batch Size:
    //Default value:
    max_ur_set_size=JURA_DEFAULT_MAX_APEL_UR_SET_SIZE;
    //From jobreport_options:
    std::string urbatch=joblog["jobreport_option_urbatch"];
    if (!urbatch.empty())
      {
	std::istringstream is(urbatch);
	is>>max_ur_set_size;
      }

  }

  void ApelDestination::report(Arc::JobLogFile &joblog)
  {
    //if (joblog.exists())
      {
        //Store copy of job log
        joblogs.push_back(joblog);
        //Create UR if can
        Arc::XMLNode usagerecord(Arc::NS(), "");
	joblog.createUsageRecord(usagerecord);
	if (usagerecord)
	  {
            std::map<std::string, std::string> usagerecord_apel;
            XML2KeyValue(usagerecord, usagerecord_apel);
            usagerecordset_apel.push_back(usagerecord_apel);
	    ++urn;
	  }
	else
	  {
	    logger.msg(Arc::INFO,"Ignoring incomplete log file \"%s\"",
		       joblog.getFilename().c_str());
	    joblog.remove();
	  }
      }

    if (urn==max_ur_set_size)
      // Batch is full. Submit and delete job log files.
      submit_batch();
  }

  void ApelDestination::finish()
  {
    if (urn>0)
      // Send the remaining URs and delete job log files.
      submit_batch();
  }

  int ApelDestination::submit_batch()
  {
    std::string urstr;
    urstr = "APEL-individual-job-message: v0.1\n";
    std::map<std::string,std::string>::iterator it;
    for (int i=0; i<(int)usagerecordset_apel.size(); i++){
        for ( it=usagerecordset_apel[i].begin() ; it != usagerecordset_apel[i].end(); it++ ){
            urstr += (*it).first + ": " + (*it).second + "\n";
        }
        urstr += "%%\n";
    }

    logger.msg(Arc::INFO, 
               "Logging UR set of %d URs.",
               urn);
    logger.msg(Arc::DEBUG, 
               "UR set dump: %s",
               urstr.c_str());
  
    // Communication with Apel server
    Arc::MCC_Status status=send_request(urstr);

    if (status.isOk())
      {
        // Delete log files
        for (std::list<JobLogFile>::iterator jp=joblogs.begin();
             jp!=joblogs.end();
             jp++
             )
          {
            (*jp).remove();
          }
        clear();
        return 0;
      }
    else // status.isnotOk
      {
        logger.msg(Arc::ERROR, 
                   "%s: %s",
                   status.getOrigin().c_str(),
                   status.getExplanation().c_str()
                   );
        clear();
        return -1;
      }
  }

  Arc::MCC_Status ApelDestination::send_request(const std::string &urset)
  {
    //Filename generation
    std::string output_path;
    output_path = output_dir;
    if (output_dir[output_dir.size()-1] != '/'){
        output_path = output_dir + "/";
    }
    output_path += "Apel_records_" + Current_Time();

    std::ifstream ifile(output_path.c_str());
    if (ifile) {
        // The file exists, and create new filename
        sequence++;
        std::stringstream ss;
        ss << sequence;
        output_path += ss.str();
    }
    else {
        sequence=0;
    }

    //Save all records into the output file.
    const char* filename(output_path.c_str());
    std::ofstream outputfile;
    outputfile.open (filename);
    if (outputfile.is_open())
    {
        outputfile << urset;
        outputfile.close();
    }
    else
    {
        return Arc::MCC_Status(Arc::PARSING_ERROR,
               "apelclient",
               std::string(
                 "Error opening file: "
                           )+ 
               filename
               );
    }

    return Arc::MCC_Status(Arc::STATUS_OK,
                           "apelclient",
                           "Output file created.");
  }

  void ApelDestination::clear()
  {
    urn=0;
    joblogs.clear();
    usagerecordset_apel.clear();
  }

  ApelDestination::~ApelDestination()
  {
    finish();
  }

  void ApelDestination::XML2KeyValue(Arc::XMLNode &xml_ur, std::map<std::string, std::string> &key_ur)
  {
    //RecordIdentity 
    //GlobalJobId 

    //LocalJobId
    if( bool(xml_ur["JobIdentity"]["LocalJobId"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("LocalJobID", xml_ur["JobIdentity"]["LocalJobId"]) );
    }

    //ProcessId 

    //LocalUserId 
    if( bool(xml_ur["JobIdentity"]["LocalUserId"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("LocalUserID", xml_ur["JobIdentity"]["LocalUserId"]) );
    } 

    //GlobalUserName 
    if( bool(xml_ur["UserIdentity"]["GlobalUserName"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("GlobalUserName", xml_ur["UserIdentity"]["GlobalUserName"]) );
    } 
    //JobName 
    //Charge 
    //Status 

    //WallDuration 
    if( bool(xml_ur["WallDuration"]) ){
        std::stringstream ss;
        ss << Arc::Period(xml_ur["WallDuration"],Arc::PeriodSeconds).GetPeriod();
        key_ur.insert ( std::pair<std::string,std::string>("WallDuration", ss.str()) );
    } 

    //CpuDuration 
    if( bool(xml_ur["CpuDuration"]) ){
        int CpuDur(0);
        //CpuDuration = CpuDuration(kernel) + CpuDuration(user)
        for (int i=0; bool(xml_ur["CpuDuration"][i]); i++){
            CpuDur += (Arc::Period(xml_ur["CpuDuration"][i],Arc::PeriodSeconds)).GetPeriod();
        }
        std::stringstream ss;
        ss << CpuDur;
        key_ur.insert ( std::pair<std::string,std::string>("CpuDuration", ss.str()) );
    } 

    //StartTime 
    if( bool(xml_ur["StartTime"]) ){
        std::stringstream ss;
        ss << Arc::Time((std::string)xml_ur["StartTime"]).GetTime();
        key_ur.insert ( std::pair<std::string,std::string>("StartTime", ss.str()) );
    } 

    //EndTime 
    if( bool(xml_ur["EndTime"]) ){
        std::stringstream ss;
        ss << Arc::Time((std::string)xml_ur["EndTime"]).GetTime();
        key_ur.insert ( std::pair<std::string,std::string>("EndTime", ss.str()) );
    } 

    //MachineName 
    if( bool(xml_ur["MachineName"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("Site", xml_ur["MachineName"]) );
    } 

    //Host 
    //SubmitHost 

    //Queue 
    if( bool(xml_ur["Queue"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("SubmitHost", xml_ur["Queue"]) );
    } 

    //ProjectName 
    //Network 
    //Disk 

    //Memory 
    if( bool(xml_ur["Memory"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("MemoryReal", xml_ur["Memory"]) );
    } 
 
    //Swap

    //Nodecount 
    if( bool(xml_ur["NodeCount"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("NodeCount", xml_ur["NodeCount"]) );
    } 

    //Processors 
    if( bool(xml_ur["Processors"]) ){
        key_ur.insert ( std::pair<std::string,std::string>("Processors", xml_ur["Processors"]) );
    } 
    //TimeDuration
    //TimeInstant
    //ServiceLevel

    //Apel's default values
    key_ur.insert ( std::pair<std::string,std::string>("ScalingFactorUnit", "custom") );
    key_ur.insert ( std::pair<std::string,std::string>("ScalingFactor", "1") );
    key_ur.insert ( std::pair<std::string,std::string>("UserFQAN", "None") );
  }

}
