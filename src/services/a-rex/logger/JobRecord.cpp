#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <istream>
#include <sstream>

#include <arc/XMLNode.h>

#include "../grid-manager/conf/conf.h"

namespace ARex {

static void split(const std::string& str, const std::string delim, Arc::XMLNode& root, const std::string& name) {
  std::string::size_type offset = 0;
  std::string::size_type delimIndex = 0;

  delimIndex = str.find(delim, offset);

  while (delimIndex != std::string::npos) {
    root.NewChild(name) = str.substr(offset, delimIndex - offset);
    offset += delimIndex - offset + delim.length();
    delimIndex = str.find(delim, offset);
  }

  root.NewChild(name) = str.substr(offset);
}

class JobRecord: public Arc::XMLNode {
 private:
  bool valid;
  void set(std::istream& i);
 public:
  std::string url;
  operator bool(void) { return valid; };
  bool operator!(void) { return !valid; };
  JobRecord(std::istream& i);
  JobRecord(const std::string& s);
  JobRecord(XMLNode& xml);
  JobRecord(const JobRecord& j);
  ~JobRecord(void);
};

JobRecord::JobRecord(const JobRecord& j) : XMLNode() {
  valid=j.valid;
  url=j.url; // ???
  j.New(*this);
}

JobRecord::JobRecord(XMLNode& xml) {
  valid=true; // !!!
  xml.New(*this);
}

void JobRecord::set(std::istream& i) {
  valid=false;
  Arc::NS ns;
  ns[""]="http://www.nordugrid.org/ws/schemas/ARCLoggerV2";
  Arc::XMLNode x(ns,"UsageRecord");
  x.New(*this);
  for(;;) {
    if(i.fail()) goto error;
    if(i.eof()) break;
    std::string value;
    std::string key = config_read_line(i,value,'=');
    if(key=="loggerurl") { url=value; }
    else if(key=="ngjobid") { NewChild("globaljobid")=value; }
    else if(key=="usersn") { NewChild("globaluserid")=value; }
    else if(key=="usersn") { NewChild("globaluserid")=value; }
    else if(key=="cluster") { NewChild("cluster")=value; }
    else if(key=="description") { NewChild("jobdescription")=value; }
    else if(key=="projectname") { NewChild("projectname")=value; }
    else if(key=="jobname") { NewChild("jobname")=value; }
    else if(key=="clienthost") { NewChild("submithost")=value; }
    else if(key=="requestedcputime") { NewChild("requestedcputime")=value; }
    else if(key=="requestedwalltime") { NewChild("requestedwalltime")=value; }
    else if(key=="requestedmemory") { NewChild("requestedmemory")=value; }
    else if(key=="requesteddisk") { NewChild("requesteddisk")=value; }
    else if(key=="submissiontime") { NewChild("submissiontime")=value; }
    else if(key=="localuser") { NewChild("localuserid")=value; }
    else if(key=="queue") { NewChild("queue")=value; }
    else if(key=="lrms") { NewChild("lrms")=value; }
    else if(key=="localjobid") { NewChild("localjobid")=value; }
    else if(key=="lrmssubmissiontime") { NewChild("lrmssubmissiontime")=value; }
    else if(key=="lrmsendtime") { NewChild("lrmsendtime")=value; }
    else if(key=="nodename") { split(value, ",", *this, "nodename"); }
    else if(key=="nodecount") { NewChild("nodecount")=value; }
    else if(key=="processors") { NewChild("processors")=value; }
    else if(key=="exitcode") { NewChild("exitcode")=value; }
    else if(key=="failurestring") { NewChild("failurestring")=value; }
    else if(key=="usedcputime") { NewChild("usedcputime")=value; }
    else if(key=="usedwalltime") { NewChild("usedwalltime")=value; }
    else if(key=="usedmemory") { NewChild("usedmemory")=value; }
    else if(key=="useddisk") { NewChild("useddisk")=value; }
    else if(key=="status") { NewChild("status")=value; }
    else if(key=="endtime") { NewChild("endtime")=value; }
    else if(key=="downloadtime") { NewChild("downloadtime")=value; }
    else if(key=="uploadtime") { NewChild("uploadtime")=value; }
    else if(key=="processid") { split(value, ",", *this, "processid"); }
    else if(key=="charge") { NewChild("charge")=value; }
    else if(key=="network") { NewChild("network")=value; }
    else if(key=="stageindata") { NewChild("stageindata")=value; }
    else if(key=="stageoutdata") { NewChild("stageoutdata")=value; }
    else if(key=="usedswap") { NewChild("usedswap")=value; }
    else if(key=="servicelevel") { NewChild("servicelevel")=value; }
    else if(key=="runtimeenvironment") { split(value, ",", *this, "runtimeenvironment"); }
  };
  valid=true;
error:
  return;
}

JobRecord::JobRecord(std::istream& i) {
  set(i);
}

JobRecord::JobRecord(const std::string& s) {
  std::istringstream i(s);
  set(i);
}

JobRecord::~JobRecord(void) {
}

}

