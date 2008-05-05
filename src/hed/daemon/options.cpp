#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

namespace Arc {

#ifdef HAVE_GLIBMM_OPTIONS

ServerOptions::ServerOptions() : Glib::OptionGroup("Server options", "server options of arc server", "help") {
    
    foreground = false;
    Glib::OptionEntry entry1;
    entry1.set_long_name("foreground");
    entry1.set_short_name('f');
#ifdef HAVE_GLIBMM_OPTIONFLAGS
    entry1.set_flags(Glib::OptionEntry::FLAG_NO_ARG | Glib::OptionEntry::FLAG_OPTIONAL_ARG);
#endif
    entry1.set_description("run daemon in foreground");
    add_entry(entry1, foreground);
    
#ifdef WIN32
    config_file = "service.xml";
#else
    config_file = "/etc/arc/server.xml";
#endif

    Glib::OptionEntry entry2;
    entry2.set_long_name("config");
    entry2.set_short_name('c');
#ifdef HAVE_GLIBMM_OPTIONFLAGS
    entry2.set_flags(Glib::OptionEntry::FLAG_FILENAME);
#endif
    entry2.set_description("full path of config file");
    add_entry_filename(entry2, config_file);
    
    pid_file = "";
    Glib::OptionEntry entry3;
    entry3.set_long_name("pid-file");
    entry3.set_short_name('p');
#ifdef HAVE_GLIBMM_OPTIONFLAGS
    entry3.set_flags(Glib::OptionEntry::FLAG_FILENAME);
#endif
    entry3.set_description("full path of pid file");
    add_entry_filename(entry3, pid_file);
#ifdef WIN32
    install = false;
    Glib::OptionEntry entry4;
    entry4.set_long_name("install");
    entry4.set_short_name('i');
#ifdef HAVE_GLIBMM_OPTIONFLAGS
    entry4.set_flags(Glib::OptionEntry::FLAG_NO_ARG | Glib::OptionEntry::FLAG_OPTIONAL_ARG);
#endif
    entry4.set_description("install windows service");
    add_entry(entry4, install);
    
    uninstall = false;
    Glib::OptionEntry entry5;
    entry5.set_long_name("uninstall");
    entry5.set_short_name('u');
#ifdef HAVE_GLIBMM_OPTIONFLAGS
    entry5.set_flags(Glib::OptionEntry::FLAG_NO_ARG | Glib::OptionEntry::FLAG_OPTIONAL_ARG);
#endif
    entry5.set_description("uninstall windows service");
    add_entry(entry5, uninstall);
#endif
}

#else

#ifndef WIN32
const char* ServerOptions::optstring = "fc:p:";
#else
const char* ServerOptions::optstring = "fc:p:iu";
#endif
const struct option ServerOptions::longopts[] = {
    {"foreground", 0, NULL, 'f'},
    {"config", 1, NULL, 'c'},
    {"pid-file", 1, NULL, 'p'},
#ifdef WIN32
    {"install", 0, NULL, 'i'},
    {"uninstall", 0, NULL, 'u'},
#endif
    {NULL, 0, NULL, 0}
};

ServerOptions::ServerOptions() {
    foreground = false;
    // config_file = "/etc/arc/server.xml";
    config_file = "service.xml";
    pid_file = "";
    user="";
    group="";
#ifdef WIW32
    install = false;
    uninstall = false;
#endif
}

int ServerOptions::parse(int argc,char * const argv[]) {
    for(;;) {
        int r = getopt_long(argc,argv,optstring,longopts,NULL);
        switch(r) {
            case -1: return 1;
            case '?': return 0;
            case ':': return 0;
            case 'f': foreground=true; break;
            case 'c': config_file=optarg; break;
            case 'p': pid_file=optarg; break;
#ifdef WIN32
	    case 'i': install = true; break;
	    case 'u': uninstall = true; break;
#endif
            default: return 0;
        };
    };
}

#endif

} // namespace Arc
