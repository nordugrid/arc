#include <iostream>
#include <arc/client/JobDescription.h>

int main() {
  std::cout << " [ JobDescription tester ] " << std::endl;
  std::cout << std::endl << " [ Parsing ] " << std::endl << std::endl;

  std::string xrsl_string = "&(executable=\"test.sh\") \
  (arguments=\"pal\") \
  (stdout=\"stdout.txt\") \
  (join=\"yes\") \
  (inputfiles=(\"myjob.sh\" \"test.sh\")) \
  (outputfiles=(\"stdout.txt\" \"download.txt\")) \
  (CPUtime=10) \
  (environment=(\"ATLAS\" \"/opt/atlas\") (\"CERN\" \"/cern\")) \
  (jobName=\"MyTestJob\")";

  std::list<Arc::JobDescription> jds;
  if (!Arc::JobDescription::Parse( xrsl_string , jds ) || jds.empty()) {
    return 1;
  }

  jds.front().Print();

  std::string jobdescstring;
  jds.front().UnParse(jobdescstring, "egee:jdl");
  std::cout << std::endl << " [ egee:jdl ] " << std::endl << std::endl << jobdescstring << std::endl;

  return 0;
}
