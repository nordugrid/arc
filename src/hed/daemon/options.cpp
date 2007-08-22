#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "options.h"

namespace Arc {

#ifdef HAVE_GLIB_OPTIONS

ServerOptions::ServerOptions() : Glib::OptionGroup("Server options", "server options of arc server", "help") {
    
    foreground = false;
    Glib::OptionEntry entry1;
    entry1.set_long_name("foreground");
    entry1.set_short_name('f');
    entry1.set_flags(Glib::OptionEntry::FLAG_NO_ARG | Glib::OptionEntry::FLAG_OPTIONAL_ARG);
    entry1.set_description("run daemon in foreground");
    add_entry(entry1, foreground);
    
    config_file = "/etc/arc/server.xml";
    Glib::OptionEntry entry2;
    entry2.set_long_name("config");
    entry2.set_short_name('c');
    entry2.set_flags(Glib::OptionEntry::FLAG_FILENAME);
    entry2.set_description("full path of config file");
    add_entry_filename(entry2, config_file);
    
    pid_file = "";
    Glib::OptionEntry entry3;
    entry3.set_long_name("pid-file");
    entry3.set_short_name('p');
    entry3.set_flags(Glib::OptionEntry::FLAG_FILENAME);
    entry3.set_description("full path of pid file");
    add_entry_filename(entry3, pid_file);
}

#else

const char* ServerOptions::optstring = "fc:p:";
const struct option ServerOptions::longopts[] = {
    {"foreground", 0, NULL, 'f'},
    {"config", 1, NULL, 'c'},
    {"pid-file", 1, NULL, 'p'},
    {NULL, 0, NULL, 0}
};

ServerOptions::ServerOptions() {
    foreground = false;
    config_file = "/etc/arc/server.xml";
    pid_file = "";
    user="";
    group="";
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
            default: return 0;
        };
    };
}

#endif

} // namespace Arc
