#ifndef __ARC_SERVER_OPTIONS_H__
#define __ARC_SERVER_OPTIONS_H__

#include <glibmm.h>

namespace Arc {

class ArcServerOptions : public Glib::OptionGroup {
    public:
        ArcServerOptions();
        
        /* Command line options values */
        bool foreground;
        std::string config_file;
        std::string pid_file;
        std::string user;
        std::string group;

}; // ArcServerOptions

};
#endif // __ARC_SERVER_OPTIONS_H__
