#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <iostream>

#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
#include <glibmm/optioncontext.h>
#else
#include <getopt.h>
#include <arc/IString.h>
#include <arc/StringConv.h>
#endif

#include <arc/misc/OptionParser.h>

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
    for (std::list<OptionBase *>::iterator it = options.begin();
	 it != options.end(); it++)
      delete *it;
  }

  void OptionParser::AddOption(const char shortOpt,
			       const std::string& longOpt,
			       const std::string& optDesc,
			       bool& value) {
    options.push_back(new BoolOption(shortOpt, longOpt,
				     optDesc, "", value));
  }

  void OptionParser::AddOption(const char shortOpt,
			       const std::string& longOpt,
			       const std::string& optDesc,
			       const std::string& argDesc,
			       int& value) {
    options.push_back(new IntOption(shortOpt, longOpt,
				    optDesc, argDesc, value));
  }

  void OptionParser::AddOption(const char shortOpt,
			       const std::string& longOpt,
			       const std::string& optDesc,
			       const std::string& argDesc,
			       std::string& value) {
    options.push_back(new StringOption(shortOpt, longOpt,
				       optDesc, argDesc, value));
  }

  void OptionParser::AddOption(const char shortOpt,
			       const std::string& longOpt,
			       const std::string& optDesc,
			       const std::string& argDesc,
			       std::list<std::string>& value) {
    options.push_back(new StringListOption(shortOpt, longOpt,
					   optDesc, argDesc, value));
  }

#ifdef HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY
  std::list<std::string> OptionParser::Parse(int argc, char **argv) {

    Glib::OptionContext ctx(arguments);
    if (!summary.empty())
      ctx.set_summary(summary);
    if (!description.empty())
      ctx.set_description(description);
    ctx.set_translation_domain("Arc");
    Glib::OptionGroup grp("main", "Main Group");
    grp.set_translation_domain("Arc");

    for (std::list<OptionBase *>::iterator it = options.begin();
	 it != options.end(); it++)
      (*it)->AddEntry(grp);

    ctx.set_main_group(grp);

    try {
      ctx.parse(argc, argv);
    }
    catch (Glib::OptionError err) {
      std::cout << err.what() << std::endl;
      exit(1);
    }

    for (std::list<OptionBase *>::iterator it = options.begin();
	 it != options.end(); it++)
      (*it)->Result();

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
    opt.name = name;
    opt.has_arg = has_arg;
    opt.flag = flag;
    opt.val = val;
  }

  std::list<std::string> OptionParser::Parse(int argc, char **argv) {

    struct option *longoptions = new struct option[options.size() + 2];
    int i = 0;
    std::string optstring;
    for (std::list<OptionBase *>::iterator it = options.begin();
	 it != options.end(); it++) {
      setopt(longoptions[i++], (*it)->longOpt.c_str(),
	     (*it)->argDesc.empty() ? no_argument : required_argument,
	     NULL, (*it)->shortOpt);
      optstring += (*it)->shortOpt;
      if (!(*it)->argDesc.empty())
	optstring += ':';
    }
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
      opt = getopt_long_only(argc, argv, optstring.c_str(), longoptions, NULL);
      if (opt == -1)
	continue;
      if (opt == '?') {
	if (optopt) {
	  delete longoptions;
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
	std::cout << "  -?, --help    " << IString("Show help options")
		  << std::endl << std::endl;
	std::cout << IString("Application Options:") << std::endl;
	for (std::list<OptionBase *>::iterator it = options.begin();
	     it != options.end(); it++) {
	  std::cout << "  -" << (*it)->shortOpt << ", --" << (*it)->longOpt;
	  if (!(*it)->argDesc.empty())
	    std::cout << "=" << IString((*it)->argDesc);
	  std::cout << "    " << IString((*it)->optDesc) << std::endl;
	}
	std::cout << std::endl;
	if (!description.empty())
	  std::cout << IString(description) << std::endl;
	delete longoptions;
	exit(0);
      }
      for (std::list<OptionBase *>::iterator it = options.begin();
	   it != options.end(); it++)
	if (opt == (*it)->shortOpt) {
	  if (!(*it)->Set(optarg ? optarg : "")) {
	    delete longoptions;
	    exit(1);
	  }
	  break;
	}
    }

    delete longoptions;
    argv[0] = argv0save;

    std::list<std::string> params;
    while (argc > optind)
      params.push_back(argv[optind++]);
    return params;
  }
#endif

} // namespace Arc
