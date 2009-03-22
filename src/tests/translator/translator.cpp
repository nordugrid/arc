#include <iostream>
#include <arc/client/JobDescription.h>

int main() {
  std::cout << " [ JobDescription tester ] " << std::endl;
  std::cout << std::endl << " [ Parsing ] " << std::endl << std::endl;

  Arc::JobDescription jd;
  std::string xrsl_string = "&(executable=\"test.sh\") \
  (arguments=\"pal\") \
  (stdout=\"stdout.txt\") \
  (join=\"yes\") \
  (inputfiles=(\"myjob.sh\" \"test.sh\")) \
  (outputfiles=(\"stdout.txt\" \"download.txt\")) \
  (CPUtime=10) \
  (environment=(\"ATLAS\" \"/opt/atlas\") (\"CERN\" \"/cern\")) \
  (jobName=\"MyTestJob\")";
  jd.Parse( xrsl_string );

  jd.Print();

  std::string jobdescstring = jd.UnParse("JDL");
  std::cout << std::endl << " [ jd.UnParse(\"JDL\") ] " << std::endl << std::endl << jobdescstring << std::endl;
}
