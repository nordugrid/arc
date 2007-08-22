#ifndef __ARC_SERVER_OPTIONS_H__
#define __ARC_SERVER_OPTIONS_H__

#ifdef HAVE_GLIBMM_OPTIONS
#include <glibmm.h>
#else
#include <unistd.h>
#include <getopt.h>
#include <string>
#endif

namespace Arc {

#ifdef HAVE_GLIBMM_OPTIONS

class ServerOptions : public Glib::OptionGroup {
    public:
        ServerOptions();
        
        /* Command line options values */
        bool foreground;
        std::string config_file;
        std::string pid_file;
        std::string user;
        std::string group;

}; // ServerOptions

#else

class ServerOptions {
    public:
        ServerOptions();
        int parse(int argc,char * const argv[]);
        
        /* Command line options values */
        bool foreground;
        std::string config_file;
        std::string pid_file;
        std::string user;
        std::string group;

    private:
        static const char* optstring;
        static const struct option longopts[];

}; // ServerOptions

#endif

} // namespace Arc

#endif // __ARC_SERVER_OPTIONS_H__
