#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"
#include <arc/StringConv.h>
#include <iostream>
#include <fstream>

namespace GridScheduler
{

Job::Job(void){
    timeout = 30;
}

Job::Job(JobRequest d, JobSchedMetaData m, int t)
{
    descr = d;
    sched_meta = m;
    sched_meta.timeout = t;
    id = make_uuid();
    status = NEW;
}

Job::Job(const std::string& jobid)
{
    id = jobid;
}

Job::Job(std::istream& job)
{

    std::string xml_document;
    std::string xml_line;
    Arc::XMLNode tmp_xml;

    while (getline(job, xml_line)) xml_document += xml_line;
    
    (Arc::XMLNode (xml_document)).New(tmp_xml);

    JobRequest job_desc(tmp_xml);
    setJobRequest(job_desc);
    status = NEW;
}

Job::~Job(void)
{
    // NOP
}

void Job::setJobRequest(JobRequest &d)
{
    descr = d;
}

void Job::setJobSchedMetaData(JobSchedMetaData &m)
{
    sched_meta = m;
}

Arc::XMLNode Job::getJSDL(void)
{
    return descr.getJSDL();
}

void Job::setArexJobID(std::string id)
{
    arex_job_id = id;
}


std::string Job::getArexJobID(void)
{
    return  arex_job_id;
}

void Job::setArexID(std::string id)
{
    sched_meta.setArexID(id);
}

std::string Job::getArexID(void)
{
    return sched_meta.getArexID();
}

bool Job::CheckTimeout(void)
{
    unsigned i = (unsigned)time(NULL) - sched_meta.last_check_time;
    std::cout << "Checktime diff: " << i << std::endl;
    if ( i  < timeout)
        return true;
    else
        return false;
}


bool SchedStatetoString(SchedStatus s, std::string &state) {

    switch (s){
    case NEW:
        state = "New";
        break;
    case STARTING:
        state = "Starting";
        break;
    case RUNNING:
        state = "Running";
        break;
    case CANCELLED:
        state = "Cancelled";
        break;
    case FAILED:
        state = "Failed";
        break;
    case FINISHED:
        state = "Finished";
        break;
    case UNKNOWN:
        state = "Unknown";
        break;
    case KILLED:
        state = "Killed";
        break;
    case KILLING:
        state = "Killing";
        break;
    default:
        return false;
    }
    return true;
}


bool StringtoSchedState(std::string &state, SchedStatus s) {

    if (state == "New")
        s = NEW;
    else if (state == "Starting")
        s = STARTING;
    else if (state == "Running")
        s = RUNNING;
    else if (state == "Cancelled")
        s = CANCELLED;
    else if (state == "Failed")
        s = FAILED;
    else if (state == "Finished")
        s = FINISHED;
    else if (state == "Unknown")
        s = UNKNOWN;
    else if (state == "Killed")
        s = KILLED;
    else if (state == "Killing")
        s = KILLING;
    else
        return false;
    return true;
}
bool ArexStatetoSchedState(std::string &arex_state, SchedStatus sched_state) {

    if(arex_state == "Accepted") {
        sched_state = STARTING;
    } else if(arex_state == "Preparing") {
        sched_state = STARTING;
    } else if(arex_state == "Submiting") {
        sched_state = STARTING;
    } else if(arex_state == "Executing") {
        sched_state = RUNNING;
    } else if(arex_state == "Finishing") {
        sched_state = RUNNING;
    } else if(arex_state == "Finished") {
        sched_state = FINISHED;
    } else if(arex_state == "Deleted") {
        sched_state = FINISHED;
    } else if(arex_state == "Killing") {
        sched_state = RUNNING;
    };

}

bool Job::Cancel(void) { 
    status = KILLED;
    return true;
}

inline void write_pair(std::ofstream &f,std::string name,std::string &value) {
    f << name << '=' << value << std::endl;
}


bool Job::save(void) { 

    // write out the jobrequest
     Arc::XMLNode jsdl = getJSDL();
     std::string jsdl_str;
     std::string fname = "/tmp/" + id + ".jsdl";
     jsdl.GetXML(jsdl_str);
     std::ofstream f1(fname.c_str(),std::ios::out | std::ios::trunc);
     if(! f1.is_open() ) return false; /* can't open file */
     f1 << jsdl_str;
     f1.close();
     
    // write out job metadata

     fname = "/tmp/" + id + ".metadata";
     std::ofstream f2(fname.c_str(),std::ios::out | std::ios::trunc);
     if(! f2.is_open() ) return false; /* can't open file */
     write_pair(f2,"id",id);
     std::string arex_id = getArexID();
     write_pair(f2,"arex_id",arex_id);
     std::string status_str;
     SchedStatetoString(status, status_str);
     write_pair(f2,"status",status_str);
     f2.close();


    // write out arex_job_id
     fname = "/tmp/" + id + ".arex_job_id";
     jsdl.GetXML(jsdl_str);
     std::ofstream f3(fname.c_str(),std::ios::out | std::ios::trunc);
     if(! f3.is_open() ) return false; /* can't open file */
     f3 << arex_job_id;
     f3.close();
}

bool Job::load(void) {
  char buf[1000];
  char name[100];
  char value[900];
  std::string fname = "/tmp/" + id + ".metadata";
  std::ifstream f(fname.c_str());
  if(! f.is_open() ) return false;
  for(;!f.eof();) {
    f.getline(buf, 1000);
    if (sscanf (buf,"%s=%s",name,value) != 2) continue;
    std::string name_str(name);
    std::string value_str(value);


    if(name_str == "id") {
        id = value_str;
    }
    else if(name_str == "arex_id") { 
        std::string job_id = value_str;
        setArexID(job_id); 
    }
    else if(name_str == "arex_job_id") {
        arex_job_id = value_str; 
    }
    else if(name_str == "status") {
        std::string s = value_str;
        StringtoSchedState(s, status);
    }
  }
  f.close(); 

  // read jsdl
  //

  std::string fname_jsdl = "/tmp/" + id + ".jsdl";
  std::ifstream f_jsdl(fname_jsdl.c_str());
  std::string xml_document;
  std::string xml_line;
  Arc::XMLNode tmp_xml;
  while (getline(f_jsdl, xml_line)) xml_document += xml_line;
  (Arc::XMLNode (xml_document)).New(tmp_xml);
  f_jsdl.close();
  JobRequest job_desc(tmp_xml);
  setJobRequest(job_desc);

  //read arex_job_id

  std::string a_id = "/tmp/" + id + ".arex_job_id";
  std::ifstream f_arex(a_id.c_str());
  std::string line;
  Arc::XMLNode tmp_xml2;
  while (getline(f_arex, line)) line += line;
  arex_job_id = line;
  f_arex.close();
  return true;
}

bool Job::remove(void) {
    std::string file1 = "/tmp/" + id + ".metadata";
    std::string file2 = "/tmp/" + id + ".jsdl";
    std::string file3 = "/tmp/" + id + ".arex_job_id";
    std::remove(file1.c_str());
    std::remove(file2.c_str());
    std::remove(file3.c_str());
}

};
