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
  jd.setSource( xrsl_string );

  std::string test;
  Arc::XMLNode node;
  //jd.getXML().GetDoc(test, true);
  if ( !jd.getXML(node) ){
       std::cerr << "The JobDescription was empty." << std::endl;
       return 1;
  }
  node.GetDoc(test, true);
  std::cout << std::endl << " [ jd.getXML() ] " << std::endl << std::endl << test << std::endl;

  try {
    std::string jobdescstring;
    jd.getProduct(jobdescstring, "JDL");
    std::cout << std::endl << " [ jd.getProduct(var, \"JDL\") ] " << std::endl << std::endl << jobdescstring << std::endl;
  } catch( Arc::JobDescriptionError jde ) {
    std::cout << jde.what() << std::endl;
    exit(1);
  }

}
