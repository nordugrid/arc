#include <sys/stat.h>
#include <fstream>
#include <arc/client/JobDescription.h>

int main(int argc, char* argv[]) {

  if (argc != 2) {
    std::cout << "Usage: test_jobdescription job_description_string_or_file" << std::endl;
    return 1;
  }

  struct stat stFileInfo;
  int intStat;
  std::string original_description;

  // Attempt to get the file attributes 
  intStat = stat(argv[1], &stFileInfo);

  if(intStat == 0) { 
    // We were able to get the file attributes 
    // so the file obviously exists. 
    std::ifstream ifs;
    std::string buffer;
    ifs.open(argv[1], std::ios::in);
    while (std::getline(ifs, buffer)) {
      original_description += buffer + "\n";
    }
  } else {
    original_description = argv[1];
  }
  Arc::JobDescription jd;

  std::cout << " [ JobDescription tester ] " << std::endl;
  std::cout << std::endl << " [ Parsing the orignal text ] " << std::endl << std::endl;
  std::cout << original_description << std::endl;
  jd.setSource( original_description );

  std::string test;
  jd.getXML().GetDoc(test, true);
  std::cout << std::endl << " [ jd.getXML() ] " << std::endl << std::endl << test << std::endl;

  jd.getProduct( test, "JSDL" );
  std::cout << std::endl << " [ JSDL ] " << std::endl << test << std::endl;
  jd.getProduct( test, "JDL" );
  std::cout << std::endl << " [ JDL ] " << std::endl << test << std::endl;
  jd.getProduct( test, "XRSL" );
  std::cout << std::endl << " [ XRSL ] " << std::endl << test << std::endl;

  return 0;
}
