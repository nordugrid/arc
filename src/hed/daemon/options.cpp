#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "options.h"

namespace Arc {

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

} // namespace Arc
