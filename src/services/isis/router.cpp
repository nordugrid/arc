#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Thread.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include "router.h"

namespace ISIS
{

RouteInfo::RouteInfo(Arc::XMLNode &node, std::list<Arc::URL> &peers_)
{
    node.New(data_);
    std::list<Arc::URL>::iterator it;
    for (it = peers_.begin(); it != peers_.end(); it++) {
        status_[(*it).fullstr()] = false;
    }
}

bool
RouteInfo::check(Arc::URL &url)
{
    return status_[url.fullstr()];
}

void
RouteInfo::update(Arc::URL &url, bool s)
{
    status_[url.fullstr()] = s;
}

bool
RouteInfo::lock(void)
{
    return mutex_.trylock();
}

void
RouteInfo::unlock(void)
{
    mutex_.unlock();
}

RouteInfo::operator bool(void)
{
    std::map<std::string, bool>::iterator it;
    int t = 0;
    for (it = status_.begin(); it != status_.end(); it++) {
        if (it->second == true) {
            t++;
        }
    }
    if (t == status_.size()) {
        return true;
    }
    return false;
}

Router::Router(Arc::Config &cfg):logger_(Arc::Logger::rootLogger, "ISIS::Router"),n_thread_(0),max_thread_(0)
{
    Arc::XMLNode r = cfg["Router"];
    // Init peers
    Arc::XMLNode peers = r["Peers"];
    Arc::XMLNode n;
    for (int i = 0; (n = peers["URL"][i]) != false; i++) {
        std::string url_str = (std::string)n;
        Arc::URL url(url_str);
        peers_.push_back(url);
    }
    std::string s = (std::string)r["MaxThread"];
    if (!s.empty()) { 
        max_thread_ = strtol(s.c_str(), NULL, 10);
    }
}

void
Router::do_route()
{
    logger_.msg(Arc::DEBUG, "Route");
    std::list<RouteInfo *>::iterator it;
    RouteInfo *ri = NULL;
    for (it = queue_.begin(); it != queue_.end(); it++) {
        ri = (*it);
        
        if (ri->lock() == false) {
            continue;
        }
        
        std::list<Arc::URL>::iterator pit;
        for (pit = peers_.begin(); pit != peers_.end(); pit++) {
            Arc::URL &url = (*pit);
            if (ri->check(url) == true) {
                continue;
            }
            std::string isis_name = url.fullstr();
            logger_.msg(Arc::DEBUG, "Routing to %s", isis_name);
            // send
            Arc::MCCConfig mcc_cfg_;
            Arc::ClientSOAP *cli_ = new Arc::ClientSOAP(mcc_cfg_, url);
            if (cli_ == NULL) {
                logger_.msg(Arc::WARNING, "Cannot create soap client");
                continue;
            }   
            Arc::PayloadSOAP request(ri->getData().Namespaces());
            Arc::XMLNode op = request.NewChild(ri->getData());
            {
                std::string s;
                op.GetXML(s);
                logger_.msg(Arc::DEBUG, s);
            }
            Arc::PayloadSOAP *response;
            Arc::MCC_Status status = cli_->process(&request, &response);
            if ((!status) || (!response)) {
                logger_.msg(Arc::ERROR, "Error during routing to %s", 
                                        isis_name);
            } else {
                Arc::XMLNode fault = (*response)["Fault"];
                if(!fault)  {
                    logger_.msg(Arc::DEBUG, "Successful routing to %s", 
                                            isis_name);      
                    // succesful send
                    ri->update(url, true);
                } else {
                    logger_.msg(Arc::DEBUG, "Failed to route to %s (%s)", 
                                       isis_name, 
                                       std::string(fault["Description"]));
                }
            }
            delete cli_;
            cli_ = NULL;
        }

        ri->unlock();
    }
    Glib::Mutex::Lock lock(mutex_);
    for (it = queue_.begin(); it != queue_.end(); it++) {
        ri = *it;
        if (ri != NULL && (bool)*ri == true) {
            it = queue_.erase(it);
            delete ri;
        } 
    }
    n_thread_--;
}

static void route_thread(void *data)
{
    Router *self = (Router *)data;
    self->do_route();
}

void
Router::route(Arc::XMLNode &node)
{
    RouteInfo *ri = new RouteInfo(node, peers_);
    queue_.push_back(ri);
    if (n_thread_ >= max_thread_) {
        return;
    }
    Arc::CreateThreadFunction(&route_thread, this);
    n_thread_++;
}

}
