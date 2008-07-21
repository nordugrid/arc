#ifndef __ARC_ISIS_ROUTER_H__
#define __ARC_ISIS_ROUTER_H__

#include <map>
#include <list>
#include <string>

#include <glibmm.h>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/Logger.h>

namespace ISIS
{

class RouteInfo
{
    private:
        Arc::XMLNode data_;
        std::map<std::string, bool> status_;
        Glib::Mutex mutex_;
    public:
        RouteInfo(Arc::XMLNode &in, std::list<Arc::URL> &peers_);
        ~RouteInfo() { };
        Arc::XMLNode &getData(void) { return data_; };
        bool check(Arc::URL &url);
        void update(Arc::URL &url, bool s);
        bool lock(void);
        void unlock(void);
        operator bool(void);
};

class Router
{
    private:
        std::list<RouteInfo *> queue_;
        std::list<Arc::URL> peers_;
        int n_thread_;
        int max_thread_;
        Arc::Logger logger_;
        Glib::Mutex mutex_;
    public:
        Router(Arc::Config &cfg);
        ~Router() {};
        void route(Arc::XMLNode &node);
        void do_route(void);
};

}
#endif
