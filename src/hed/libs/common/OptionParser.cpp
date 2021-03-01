// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <iostream>
#include <arc/IString.h>

#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
#include <glibmm/optioncontext.h>
#else
#include <getopt.h>
#include <arc/StringConv.h>
#endif

#include "OptionParser.h"

#define ARC_OPTION_DEFAULT_GROUP "default"

namespace Arc {

#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
  class OptionBase {
  public:
    OptionBase(char shortOpt, const std::string& longOpt,
               const std::string& optDesc, const std::string& argDesc) {
      entry.set_short_name(shortOpt);
      entry.set_long_name(longOpt);
      entry.set_description(optDesc);
      if (!argDesc.empty())
        entry.set_arg_description(argDesc);
    }
    virtual ~OptionBase() {}
    virtual void AddEntry(Glib::OptionGroup& grp) = 0;
    virtual void Result() {}

  protected:
    Glib::OptionEntry entry;
  };
#else
  class OptionBase {
  public:
    OptionBase(char shortOpt, const std::string& longOpt,
               const std::string& optDesc, const std::string& argDesc)
      : shortOpt(shortOpt),
        longOpt(longOpt),
        optDesc(optDesc),
        argDesc(argDesc) {}
    virtual ~OptionBase() {}
    virtual bool Set(const std::string& val) = 0;

  protected:
    char shortOpt;
    std::string longOpt;
    std::string optDesc;
    std::string argDesc;

    friend class OptionParser;
  };
#endif


  class BoolOption
    : public OptionBase {
  public:
    BoolOption(char shortOpt, std::string longOpt,
               std::string optDesc, std::string argDesc,
               bool& value)
      : OptionBase(shortOpt, longOpt, optDesc, argDesc),
        value(value) {}
    ~BoolOption() {}
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    void AddEntry(Glib::OptionGroup& grp) {
      grp.add_entry(entry, value);
    }
#else
    bool Set(const std::string&) {
      value = true;
      return true;
    }
#endif

  private:
    bool& value;
  };

  struct IntOption
    : public OptionBase {
  public:
    IntOption(char shortOpt, std::string longOpt,
              std::string optDesc, std::string argDesc,
              int& value)
      : OptionBase(shortOpt, longOpt, optDesc, argDesc),
        value(value) {}
    ~IntOption() {}
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    void AddEntry(Glib::OptionGroup& grp) {
      grp.add_entry(entry, value);
    }
#else
    bool Set(const std::string& val) {
      bool ok = stringto(val, value);
      if (!ok)
        std::cout << IString("Cannot parse integer value '%s' for -%c",
                             val, shortOpt) << std::endl;
      return ok;
    }
#endif

  private:
    int& value;
  };

  struct StringOption
    : public OptionBase {
  public:
    StringOption(char shortOpt, std::string longOpt,
                 std::string optDesc, std::string argDesc,
                 std::string& value)
      : OptionBase(shortOpt, longOpt, optDesc, argDesc),
        value(value) {}
    ~StringOption() {}
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    void AddEntry(Glib::OptionGroup& grp) {
      grp.add_entry(entry, gvalue);
    }
    void Result() {
      if (!gvalue.empty())
        value = gvalue;
    }
#else
    bool Set(const std::string& val) {
      value = val;
      return true;
    }
#endif

  private:
    std::string& value;
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    Glib::ustring gvalue;
#endif
  };

  struct StringListOption
    : public OptionBase {
  public:
    StringListOption(char shortOpt, std::string longOpt,
                     std::string optDesc, std::string argDesc,
                     std::list<std::string>& value)
      : OptionBase(shortOpt, longOpt, optDesc, argDesc),
        value(value) {}
    ~StringListOption() {}
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    void AddEntry(Glib::OptionGroup& grp) {
      grp.add_entry(entry, gvalue);
    }
    void Result() {
      value.insert(value.end(), gvalue.begin(), gvalue.end());
    }
#else
    bool Set(const std::string& val) {
      value.push_back(val);
      return true;
    }
#endif

  private:
    std::list<std::string>& value;
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
    Glib::OptionGroup::vecustrings gvalue;
#endif
  };

  OptionParser::OptionParser(const std::string& arguments,
                             const std::string& summary,
                             const std::string& description)
    : arguments(arguments),
      summary(summary),
      description(description) {}

  OptionParser::~OptionParser() {
    std::list<std::pair<OptionBase*, std::string> >::iterator it = options.begin();
    while ( it != options.end() ) {
        delete it->first;
        it->first = NULL;
        it = options.erase(it);
    }
  }

  void OptionParser::AddOption(const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               bool& value) {
    GroupAddOption(ARC_OPTION_DEFAULT_GROUP, shortOpt, longOpt, optDesc, value);
  }

  void OptionParser::AddOption(const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               int& value) {
    GroupAddOption(ARC_OPTION_DEFAULT_GROUP, shortOpt, longOpt, optDesc, argDesc, value);
  }

  void OptionParser::AddOption(const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               std::string& value) {
    GroupAddOption(ARC_OPTION_DEFAULT_GROUP, shortOpt, longOpt, optDesc, argDesc, value);
  }

  void OptionParser::AddOption(const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               std::list<std::string>& value) {
    GroupAddOption(ARC_OPTION_DEFAULT_GROUP, shortOpt, longOpt, optDesc, argDesc, value);
  }

  void OptionParser::GroupAddOption(const std::string groupID,
                               const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               bool& value) {
    optgroups[groupID];
    options.push_back(std::pair<OptionBase*, std::string>
      (new BoolOption(shortOpt, longOpt, optDesc, "", value), groupID));
  }

  void OptionParser::GroupAddOption(const std::string groupID,
                               const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               int& value) {
    optgroups[groupID];
    options.push_back(std::pair<OptionBase*, std::string>
      (new IntOption(shortOpt, longOpt,optDesc, argDesc, value), groupID));
  }

  void OptionParser::GroupAddOption(const std::string groupID,
                               const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               std::string& value) {
    optgroups[groupID];
    options.push_back(std::pair<OptionBase*, std::string>
      (new StringOption(shortOpt, longOpt, optDesc, argDesc, value), groupID));
  }

  void OptionParser::GroupAddOption(const std::string groupID,
                               const char shortOpt,
                               const std::string& longOpt,
                               const std::string& optDesc,
                               const std::string& argDesc,
                               std::list<std::string>& value) {
    optgroups[groupID];
    options.push_back(std::pair<OptionBase*, std::string>
      (new StringListOption(shortOpt, longOpt,optDesc, argDesc, value), groupID));
  }

  void OptionParser::DefineOptionsGroup(const std::string groupID, const std::string& groupName) {
    optgroups[groupID] = groupName;
  }


#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
  std::list<std::string> OptionParser::Parse(int argc, char **argv) {

    if (argc > 0) {
      origcmdwithargs = argv[0];
      for (int i = 1; i < argc; ++i) {
        origcmdwithargs += " " + std::string(argv[i]);
      }
    }

    Glib::OptionContext ctx(arguments);
    if (!summary.empty())
      ctx.set_summary(summary);
    if (!description.empty())
      ctx.set_description(description);
    ctx.set_translation_domain(PACKAGE);

#ifdef HAVE_GLIBMM_OPTIONCONTEXT_GET_HELP
    // if get_help() is supported disable native -? --help processing 
    // etc to show --help-all by default
    ctx.set_help_enabled(false);
#endif

    bool h_value = false;
    BoolOption h_entry('h', "help", "Show help options", "", h_value);

    for (std::map<std::string, std::string>::iterator g_it = optgroups.begin();
         g_it != optgroups.end(); g_it++) {

      Glib::OptionGroup *grp;

      if ( g_it->first == ARC_OPTION_DEFAULT_GROUP ) {
          grp = new Glib::OptionGroup(ARC_OPTION_DEFAULT_GROUP, "Main Group");
          h_entry.AddEntry(*grp);
      } else {
          std::string grp_name;
          if (g_it->second.empty()) {
             grp_name = IString("Options Group %s:", g_it->first).str();
          } else {
             grp_name = IString("%s:", g_it->second).str();
          }
          grp = new Glib::OptionGroup(g_it->first, grp_name, IString("Show %s help options", g_it->first).str());
      }
      grp->set_translation_domain(PACKAGE);

      bool options_in_group = false;
      for (std::list<std::pair<OptionBase*, std::string> >::iterator it = options.begin();
           it != options.end(); it++) {
        if ( (*it).second == g_it->first ) {
          options_in_group = true;
          (*it).first->AddEntry(*grp);
        }
      }

      if ( g_it->first == ARC_OPTION_DEFAULT_GROUP ) {
        ctx.set_main_group(*grp);
      } else {
        if ( options_in_group ) ctx.add_group(*grp);
      }
    }

    try {
      ctx.parse(argc, argv);
    } catch (const Glib::OptionError& err) {
      try {
        std::cerr << err.what() << std::endl;
      } catch (const Glib::Exception& err2) {
        std::cerr << IString("Failed to parse command line options") << std::endl;
      }
      exit(1);
    }
    if(h_value) {
#ifdef HAVE_GLIBMM_OPTIONCONTEXT_GET_HELP
      // In some versions Glib's get_help returns text which fails in Glib's internal translator of operator<<.
      // Hence using internal string without additional translation.
      std::cout << ctx.get_help(false).c_str() << std::endl;
#else
      std::cout << IString("Use -? to get usage description") << std::endl;
#endif
      exit(0);
    }

    for (std::list<std::pair<OptionBase*, std::string> >::iterator it = options.begin();
         it != options.end(); it++)
      (*it).first->Result();

    std::list<std::string> params;
    for (int i = 1; i < argc; i++)
      params.push_back(argv[i]);
    return params;
  }
#else
  static inline void setopt(struct option& opt,
                            const char *name,
                            int has_arg,
                            int *flag,
                            int val) {
    opt.name = (char*)name; // for buggy getopt header - Solaris
    opt.has_arg = has_arg;
    opt.flag = flag;
    opt.val = val;
  }

  std::list<std::string> OptionParser::Parse(int argc, char **argv) {

    if (argc > 0) {
      origcmdwithargs = argv[0];
      for (int i = 1; i < argc; ++i) {
        origcmdwithargs += " " + std::string(argv[i]);
      }
    }

    struct option *longoptions = new struct option[options.size() + 3];
    int i = 0;
    std::string optstring;
    for (std::list<std::pair<OptionBase*,std::string> >::iterator it = options.begin();
         it != options.end(); it++) {
      setopt(longoptions[i], (*it).first->longOpt.c_str(),
             (*it).first->argDesc.empty() ? no_argument : required_argument,
             NULL, (*it).first->shortOpt?(*it).first->shortOpt:(i+0x100));
      if((*it).first->shortOpt) {
        optstring += (*it).first->shortOpt;
        if (!(*it).first->argDesc.empty()) optstring += ':';
      }
      ++i;
    }
    setopt(longoptions[i++], "help", no_argument, NULL, 'h');
    optstring += 'h';
    setopt(longoptions[i++], "help", no_argument, NULL, '?');
    optstring += '?';
    setopt(longoptions[i++], NULL, no_argument, NULL, '\0');

    char *argv0save = argv[0];
    argv[0] = strrchr(argv[0], '/');
    if (argv[0])
      argv[0]++;
    else
      argv[0] = argv0save;

    int opt = 0;
    while (opt != -1) {
#ifdef HAVE_GETOPT_LONG_ONLY
      opt = getopt_long_only(argc, argv, optstring.c_str(), longoptions, NULL);
#else
      opt = getopt_long(argc, argv, optstring.c_str(), longoptions, NULL);
#endif

      if (opt == -1)
        continue;
      if ((opt == '?') || (opt == ':') || (opt == 'h')) {
        if (optopt) {
          delete[] longoptions;
          exit(1);
        }
        std::cout << IString("Usage:") << std::endl;
        std::cout << "  " << argv[0];
        if (!options.empty())
          std::cout << " [" << IString("OPTION...") << "]";
        if (!arguments.empty())
          std::cout << " " << IString(arguments);
        std::cout << std::endl << std::endl;
        if (!summary.empty())
          std::cout << IString(summary) << std::endl << std::endl;
        std::cout << IString("Help Options:") << std::endl;
        std::cout << "  -h, -?, --help    " << IString("Show help options")
                  << std::endl << std::endl;
        for (std::map<std::string, std::string>::iterator g_it = optgroups.begin();
             g_it != optgroups.end(); g_it++) {
          std::string group_header = "Application Options:";
          if (g_it->second.empty()) {
            if ( g_it->first != ARC_OPTION_DEFAULT_GROUP ) {
              group_header = IString("Options Group %s:", g_it->first).str();
            }
          } else {
            group_header = IString("%s:", g_it->second).str();
          }
          bool print_header = true;
          for (std::list<std::pair<OptionBase*,std::string> >::iterator it = options.begin();
               it != options.end(); it++) {
            // skip options in other groups
            if ( (*it).second != g_it->first) continue;
            // print header only if at least one option in this group
            if (print_header) {
              print_header = false;
              std::cout << group_header << std::endl;
            }
            // print option description
            std::cout << "  ";
            OptionBase* ob = (*it).first;
            if (ob->shortOpt) {
              std::cout << "-" << ob->shortOpt << ", ";
            }
            std::cout << "--";
            if (ob->argDesc.empty())
              std::cout << std::setw(20+4*(ob->shortOpt == 0)) << std::left << ob->longOpt;
            else
              std::cout << ob->longOpt << "=" << std::setw(19-(ob->longOpt).length()+((ob->shortOpt == 0))*4) << std::left << IString(ob->argDesc);
            std::cout << "  " << IString(ob->optDesc) << std::endl;
          }
        }
        std::cout << std::endl;
        if (!description.empty())
          std::cout << IString(description) << std::endl;
        delete[] longoptions;
        exit(0);
      }
      i = 0;
      for (std::list<std::pair<OptionBase*,std::string> >::iterator it = options.begin();
           it != options.end(); it++) {
        int o = (*it).first->shortOpt;
        if(!o) o = longoptions[i].val;
        if (opt == o) {
          if (!(*it).first->Set(optarg ? optarg : "")) {
            delete[] longoptions;
            exit(1);
          }
          break;
        }
        ++i;
      }
    }

    delete[] longoptions;
    argv[0] = argv0save;

    std::list<std::string> params;
    while (argc > optind)
      params.push_back(argv[optind++]);
    return params;
  }
#endif

} // namespace Arc
