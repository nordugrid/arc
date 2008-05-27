#ifndef __ARC_OPTION_H__
#define __ARC_OPTION_H__

#include <list>
#include <string>

namespace Arc {

  class OptionBase;

  class OptionParser {

  public:
    OptionParser(const std::string& arguments = "",
		 const std::string& summary = "",
		 const std::string& description = "");

    ~OptionParser();

    void AddOption(const char shortOpt,
		   const std::string& longOpt,
		   const std::string& optDesc,
		   bool& val);

    void AddOption(const char shortOpt,
		   const std::string& longOpt,
		   const std::string& optDesc,
		   const std::string& argDesc,
		   int& val);

    void AddOption(const char shortOpt,
		   const std::string& longOpt,
		   const std::string& optDesc,
		   const std::string& argDesc,
		   std::string& val);

    void AddOption(const char shortOpt,
		   const std::string& longOpt,
		   const std::string& optDesc,
		   const std::string& argDesc,
		   std::list<std::string>& val);

    std::list<std::string> Parse(int argc, char **argv);

  private:
    std::string arguments;
    std::string summary;
    std::string description;
    std::list<OptionBase *> options;
  };

} // namespace Arc

#endif // __ARC_OPTION_H__
