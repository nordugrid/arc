// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fstream>
#include <arc/client/JobDescription.h>
#include <arc/OptionParser.h>
#include <arc/IString.h>
#include <arc/Logger.h>

int main(int argc, char **argv) {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::OptionParser options(istring("[job description ...]"),
                            istring("This tiny tool can be used for testing"
                                    "the JobDescription's conversion abilities."),
                            istring("The job description also can be a file or a string in JDL, POSIX JSDL, JSDL, or XRSL format."));

  std::string requested_format = "";
  options.AddOption('f', "format",
                    istring("define the requested format (ARCJSDL, JDL, XRSL)"),
                    istring("format"),
                    requested_format);

  bool show_original_description = false;
  options.AddOption('o', "original",
                    istring("show the original job description"),
                    show_original_description);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  std::list<std::string> descriptions = options.Parse(argc, argv);
  if (descriptions.empty()) {
    std::cout << "Use --help option for detailed usage information" << std::endl;
    return 1;
  }

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  std::cout << " [ JobDescription tester ] " << std::endl;

  for (std::list<std::string>::iterator it = descriptions.begin(); it != descriptions.end(); it++) {
    struct stat stFileInfo;
    int intStat;
    std::string original_description;

    // Attempt to get the file attributes
    intStat = stat((*it).c_str(), &stFileInfo);

    if (intStat == 0) {
      // We were able to get the file attributes
      // so the file obviously exists.
      std::ifstream ifs;
      std::string buffer;
      ifs.open((*it).c_str(), std::ios::in);
      while (std::getline(ifs, buffer))
        original_description += buffer + "\n";
    }
    else
      original_description = (*it);

    if (requested_format == "JDL" || requested_format == "ARCJSDL" || requested_format == "XRSL" || requested_format == "") {
      if (show_original_description) {
        std::cout << std::endl << " [ Parsing the orignal text ] " << std::endl << std::endl;
        std::cout << original_description << std::endl;
      }

      std::list<Arc::JobDescription> jds;
      if (!Arc::JobDescription::Parse(original_description, jds) || jds.empty()) {
        std::cout << "Unable to parse." << std::endl;
        return 1;
      }

      std::string test;
      if (requested_format == "")
        jds.front().Print(true);

      if (requested_format == "JDL" || requested_format == "") {
        test = jds.front().UnParse("JDL");
        std::cout << std::endl << " [ JDL ] " << std::endl << test << std::endl;
      }

      if (requested_format == "XRSL" || requested_format == "") {
        test = jds.front().UnParse("XRSL");
        std::cout << std::endl << " [ XRSL ] " << std::endl << test << std::endl;
      }

      if (requested_format == "ARCJSDL" || requested_format == "") {
        test = jds.front().UnParse("ARCJSDL");
        std::cout << std::endl << " [ ARCJSDL ] " << std::endl << test << std::endl;
      }
    }
  }

  return 0;

}
