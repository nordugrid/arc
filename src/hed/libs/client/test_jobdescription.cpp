#include <sys/stat.h>
#include <fstream>
#include <arc/client/JobDescription.h>
#include <arc/OptionParser.h>
#include <arc/IString.h>

int main(int argc, char* argv[]) {

Arc::OptionParser options(istring("[job description ...]"),
  istring("This tiny tool can be used for testing"
          "the JobDescription's conversion abilities."),
  istring("The job description also can be a file or a string in JDL, JSDL, or XRSL format.")
  );

std::string requested_format = "";
  options.AddOption('f', "format",
    istring("define the requested format (JSDL, JDL, XRSL)"),
    istring("format"),
    requested_format);

bool show_original_description = false;
  options.AddOption('o', "original",
    istring("show the original job description"),
    show_original_description);

std::list<std::string> descriptions = options.Parse(argc, argv);
  if (descriptions.empty()) {
    std::cout << "Use --help option for detailed usage information" << std::endl;
    return 1;
  }

std::cout << " [ JobDescription tester ] " << std::endl;

for (std::list< std::string >::iterator it = descriptions.begin(); it != descriptions.end(); it++) {
  struct stat stFileInfo;
  int intStat;
  std::string original_description;

  // Attempt to get the file attributes 
  intStat = stat((*it).c_str(), &stFileInfo);

  if(intStat == 0) { 
    // We were able to get the file attributes 
    // so the file obviously exists. 
    std::ifstream ifs;
    std::string buffer;
    ifs.open((*it).c_str(), std::ios::in);
    while (std::getline(ifs, buffer)) {
      original_description += buffer + "\n";
    }
  } else {
    original_description = (*it);
  }

  if (requested_format == "JDL" || requested_format == "JSDL" || requested_format == "") {
    Arc::JobDescription jd;

    if (show_original_description) {
      std::cout << std::endl << " [ Parsing the orignal text ] " << std::endl << std::endl;
      std::cout << original_description << std::endl;
    }

    jd.setSource( original_description );

    std::string test;
    if (requested_format == "") {
      jd.getXML().GetDoc(test, true);
      std::cout << std::endl << " [ Inner representation ] " << std::endl << std::endl << test << std::endl;
    }

    if (requested_format == "JSDL" || requested_format == "") {
      try {
        jd.getProduct( test, "JSDL" );
        Arc::XMLNode jsdl(test);
        jsdl.GetXML(test, true);
        std::cout << std::endl << " [ JSDL ] " << std::endl << test << std::endl;
      } catch (Arc::JobDescriptionError jde) {
        std::cout << std::endl << " " << jde.what() << std::endl;
      }
    }

    if (requested_format == "JDL" || requested_format == "") {
      try {
        jd.getProduct( test, "JDL" );
        std::cout << std::endl << " [ JDL ] " << std::endl << test << std::endl;
      } catch (Arc::JobDescriptionError jde) {
        std::cout << std::endl << " " << jde.what() << std::endl;
      }
    }

    if (requested_format == "XRSL" || requested_format == "") {
      try {
    jd.getProduct( test, "XRSL" );
    std::cout << std::endl << " [ XRSL ] " << std::endl << test << std::endl;
      } catch (Arc::JobDescriptionError jde) {
        std::cout << std::endl << " " << jde.what() << std::endl;
      }
    }
  }
}
  return 0;

}
