// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_OPTION_H__
#define __ARC_OPTION_H__

#include <list>
#include <string>

namespace Arc {

  class OptionBase;

  /// Command line option parser used by ARC command line tools.
  /**
   * The command line arguments and a brief and detailed description can be set
   * in the constructor. Each command line option should be added with an
   * AddOption() method, corresponding to the type of the option. Parse()
   * can then be called with the same arguments as main() takes. It returns a
   * list of arguments and fills each "val" passed in AddOption() if the
   * corresponding option is specified on the command line.
   *
   * A help text is automatically generated and displayed on stdout if a help
   * option (-h or -?) is used on the command line. Note that Parse() calls
   * exit(0) after displaying the help text.
   *
   * Both short and long format options are supported.
   * \headerfile OptionParser.h arc/OptionParser.h
   */
  class OptionParser {

  public:
    /// Create a new OptionParser.
    /**
     * @param arguments Command line arguments
     * @param summary Brief summary of command
     * @param description Detailed description of command
     */
    OptionParser(const std::string& arguments = "",
                 const std::string& summary = "",
                 const std::string& description = "");

    ~OptionParser();

    /// Add an option which does not take any arguments.
    /**
     * @param shortOpt Short version of this option
     * @param longOpt Long version of this option
     * @param optDesc Description of option
     * @param val Value filled during Parse()
     */
    void AddOption(const char shortOpt,
                   const std::string& longOpt,
                   const std::string& optDesc,
                   bool& val);

    /// Add an option which takes an integer argument.
    /**
     * @param shortOpt Short version of this option
     * @param longOpt Long version of this option
     * @param optDesc Description of option
     * @param argDesc Value of option argument
     * @param val Value filled during Parse()
     */
    void AddOption(const char shortOpt,
                   const std::string& longOpt,
                   const std::string& optDesc,
                   const std::string& argDesc,
                   int& val);

    /// Add an option which takes a string argument.
    /**
     * @param shortOpt Short version of this option
     * @param longOpt Long version of this option
     * @param optDesc Description of option
     * @param argDesc Value of option argument
     * @param val Value filled during Parse()
     */
    void AddOption(const char shortOpt,
                   const std::string& longOpt,
                   const std::string& optDesc,
                   const std::string& argDesc,
                   std::string& val);

    /// Add an option which takes a string argument and can be specified multiple times.
    /**
     * @param shortOpt Short version of this option
     * @param longOpt Long version of this option
     * @param optDesc Description of option
     * @param argDesc Value of option argument
     * @param val Value filled during Parse()
     */
    void AddOption(const char shortOpt,
                   const std::string& longOpt,
                   const std::string& optDesc,
                   const std::string& argDesc,
                   std::list<std::string>& val);

    /// Parse the options and arguments.
    /**
     * Should be called after all options have been added with AddOption().
     * The parameters can be the same as those taken by main(). Note that if a
     * help option is given this method calls exit(0) after printing help text
     * to stdout.
     * @return The list of command line arguments
     */
    std::list<std::string> Parse(int argc, char **argv);

  private:
    std::string arguments;
    std::string summary;
    std::string description;
    std::list<OptionBase*> options;
  };

} // namespace Arc

#endif // __ARC_OPTION_H__
