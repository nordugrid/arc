#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <unistd.h>

// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#define ErrNo errno

#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "MCCTCP.h"

#define PROTO_NAME(ADDR) ((ADDR->ai_family==AF_INET6)?"IPv6":"IPv4")
Arc::Logger ArcMCCTCP::MCC_TCP::logger(Arc::Logger::getRootLogger(), "MCC.TCP");

ArcMCCTCP::MCC_TCP::MCC_TCP(Arc::Config *cfg, PluginArgument* parg) : Arc::MCC(cfg, parg) {
}

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    ArcMCCTCP::MCC_TCP_Service* plugin = new ArcMCCTCP::MCC_TCP_Service((Arc::Config*)(*mccarg),mccarg);
    if(!(*plugin)) {
        delete plugin;
        return NULL;
    };
    return plugin;
}

static Arc::Plugin* get_mcc_client(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    ArcMCCTCP::MCC_TCP_Client* plugin =  new ArcMCCTCP::MCC_TCP_Client((Arc::Config*)(*mccarg),mccarg);
    if(!(*plugin)) {
        delete plugin;
        return NULL;
    };
    return plugin;
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "tcp.service", "HED:MCC", NULL, 0, &get_mcc_service },
    { "tcp.client",  "HED:MCC", NULL, 0, &get_mcc_client  },
    { NULL, NULL, NULL, 0, NULL }
};


namespace ArcMCCTCP {

using namespace Arc;


MCC_TCP_Service::MCC_TCP_Service(Config *cfg, PluginArgument* parg):MCC_TCP(cfg,parg),valid_(false),max_executers_(-1),max_executers_drop_(false) {
    for(int i = 0;;++i) {
        struct addrinfo hint;
        struct addrinfo *info = NULL;
        memset(&hint, 0, sizeof(hint));
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP; // ?
        hint.ai_flags = AI_PASSIVE;
        XMLNode l = (*cfg)["Listen"][i];
        if(!l) break;
        std::string port_s = l["Port"];
        if(port_s.empty()) {
            logger.msg(ERROR, "Missing Port in Listen element");
            return;
        };
        std::string interface_s = l["Interface"];
        std::string version_s = l["Version"];
        if(!version_s.empty()) {
            if(version_s == "4") { hint.ai_family = AF_INET; }
            else if(version_s == "6") { hint.ai_family = AF_INET6; }
            else {
                logger.msg(ERROR, "Version in Listen element can't be recognized");
                return;
            };
        };
        int ret = getaddrinfo(interface_s.empty()?NULL:interface_s.c_str(),
                              port_s.c_str(), &hint, &info);
        if (ret != 0) {
            std::string err_str = gai_strerror(ret);
            if(interface_s.empty()) {
              logger.msg(ERROR, "Failed to obtain local address for port %s - %s", port_s, err_str);
            } else {
              logger.msg(ERROR, "Failed to obtain local address for %s:%s - %s", interface_s, port_s, err_str);
            };
            return;
        };
        bool bound = false;
        for(struct addrinfo *info_ = info;info_;info_=info_->ai_next) {
            if(interface_s.empty()) {
              logger.msg(VERBOSE, "Trying to listen on TCP port %s(%s)", port_s, PROTO_NAME(info_));
            } else {
              logger.msg(VERBOSE, "Trying to listen on %s:%s(%s)", interface_s, port_s, PROTO_NAME(info_));
            };
            int s = ::socket(info_->ai_family,info_->ai_socktype,info_->ai_protocol);
            if(s == -1) {
                std::string e = StrError(errno);
                if(interface_s.empty()) {
                  logger.msg(ERROR, "Failed to create socket for listening at TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                } else {
                  logger.msg(ERROR, "Failed to create socket for listening at %s:%s(%s): %s", interface_s, port_s, PROTO_NAME(info_),e);
                };
                continue;
            };
            // Set REUSEADDR so that after crash re-binding works despite TIME_WAIT sockets
            int resuseaddr_arg = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &resuseaddr_arg, sizeof(resuseaddr_arg));
#ifdef IPV6_V6ONLY
            if(info_->ai_family == AF_INET6) {
              int v = 1;
              // Some systems (Linux for example) make v6 support v4 too
              // by default. Some don't. Make it same for everyone -
              // separate sockets for v4 and v6.
              if(setsockopt(s,IPPROTO_IPV6,IPV6_V6ONLY,(const char*)(&v),sizeof(v)) != 0) {
                if(interface_s.empty()) {
                  logger.msg(ERROR, "Failed to limit socket to IPv6 at TCP port %s - may cause errors for IPv4 at same port", port_s);
                } else {
                  logger.msg(ERROR, "Failed to limit socket to IPv6 at %s:%s - may cause errors for IPv4 at same port", interface_s, port_s);
                };
              };
            };
#endif
            if(::bind(s,info_->ai_addr,info_->ai_addrlen) == -1) {
                std::string e = StrError(errno);
                if(interface_s.empty()) {
                  logger.msg(ERROR, "Failed to bind socket for TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                } else {
                  logger.msg(ERROR, "Failed to bind socket for %s:%s(%s): %s", interface_s, port_s, PROTO_NAME(info_),e);
                };
                close(s);
                if(l["AllAddresses"]) {
                  std::string v = l["AllAddresses"];
                  if((v == "false") || (v == "0")) {
                    continue;
                  };
                };
                bound = false;
                break;
            };
            if(::listen(s,-1) == -1) {
                std::string e = StrError(errno);
                if(interface_s.empty()) {
                  logger.msg(WARNING, "Failed to listen at TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                } else {
                  logger.msg(WARNING, "Failed to listen at %s:%s(%s): %s", interface_s, port_s, PROTO_NAME(info_),e);
                };
                close(s);
                continue;
            };
            bool no_delay = false;
            if(l["NoDelay"]) {
                std::string v = l["NoDelay"];
                if((v == "true") || (v == "1")) no_delay=true;
            }
            int timeout = 60;
            if (l["Timeout"]) {
                std::string v = l["Timeout"];
                timeout = atoi(v.c_str());
            }
            handles_.push_back(mcc_tcp_handle_t(s,timeout,no_delay));
            if(interface_s.empty()) {
              logger.msg(INFO, "Listening on TCP port %s(%s)", port_s, PROTO_NAME(info_));
            } else {
              logger.msg(INFO, "Listening on %s:%s(%s)", interface_s, port_s, PROTO_NAME(info_));
            };
            bound = true;
        };
        freeaddrinfo(info);
        if(!bound) {
            if(version_s.empty()) {
                logger.msg(ERROR, "Failed to start listening on any address for %s:%s", interface_s, port_s);
            } else {
                logger.msg(ERROR, "Failed to start listening on any address for %s:%s(IPv%s)", interface_s, port_s, version_s);
            };
            return;
        };
    };
    if(handles_.size() == 0) {
        logger.msg(ERROR, "No listening ports initiated");
        return;
    };
    if((*cfg)["Limit"]) {
      std::string v = (*cfg)["Limit"];
      max_executers_ = atoi(v.c_str());
      v=(std::string)((*cfg)["Limit"].Attribute("drop"));
      if((v == "yes") || (v == "true") || (v == "1")) {
        max_executers_drop_=true;
      };
      if(max_executers_ > 0) {
        logger.msg(INFO, "Setting connections limit to %i, connections over limit will be %s",max_executers_,max_executers_drop_?istring("dropped"):istring("put on hold"));
      };
    };
    if(!CreateThreadFunction(&listener,this)) {
        logger.msg(ERROR, "Failed to start thread for listening");
        for(std::list<mcc_tcp_handle_t>::iterator i = handles_.begin();i!=handles_.end();i=handles_.erase(i)) ::close(i->handle);
    };
    valid_ = true;
}

MCC_TCP_Service::~MCC_TCP_Service(void) {
    //logger.msg(VERBOSE, "TCP_Service destroy");
    lock_.lock();
    for(std::list<mcc_tcp_handle_t>::iterator i = handles_.begin();i!=handles_.end();++i) {
        ::close(i->handle); i->handle=-1;
    };
    for(std::list<mcc_tcp_exec_t>::iterator e = executers_.begin();e != executers_.end();++e) {
        ::shutdown(e->handle,2);
    };
    if(!valid_) {
        for(std::list<mcc_tcp_handle_t>::iterator i = handles_.begin();i!=handles_.end();i=handles_.erase(i)) { };
    };
    // Wait for threads to exit
    while(executers_.size() > 0) {
        lock_.unlock(); sleep(1); lock_.lock();
    };
    while(handles_.size() > 0) {
        lock_.unlock(); sleep(1); lock_.lock();
    };
    lock_.unlock();
}

MCC_TCP_Service::mcc_tcp_exec_t::mcc_tcp_exec_t(MCC_TCP_Service* o,int h,int t,bool nd):obj(o),handle(h),no_delay(nd),timeout(t) {
    if(handle == -1) return;
    // list is locked externally
    std::list<mcc_tcp_exec_t>::iterator e = o->executers_.insert(o->executers_.end(),*this);
    if(!CreateThreadFunction(&MCC_TCP_Service::executer,&(*e))) {
        logger.msg(ERROR, "Failed to start thread for communication");
        ::shutdown(handle,2);
        ::close(handle);  handle=-1; o->executers_.erase(e);
    };
}

void MCC_TCP_Service::listener(void* arg) {
    MCC_TCP_Service& it = *((MCC_TCP_Service*)arg);
    for(;;) {
        int max_s = -1;
        fd_set readfds;
        FD_ZERO(&readfds);
        it.lock_.lock();
        for(std::list<mcc_tcp_handle_t>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
            int s = i->handle;
            if(s == -1) { i=it.handles_.erase(i); continue; };
            FD_SET(s,&readfds);
            if(s > max_s) max_s = s;
            ++i;
        };
        it.lock_.unlock();
        if(max_s == -1) break;
        struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
        int n = select(max_s+1,&readfds,NULL,NULL,&tv);
        if(n < 0) {
            if(ErrNo != EINTR) {
                logger.msg(ERROR, "Failed while waiting for connection request");
                it.lock_.lock();
                for(std::list<mcc_tcp_handle_t>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
                    int s = i->handle;
                    ::close(s);
                    i=it.handles_.erase(i);
                };
                it.lock_.unlock();
                return;
            };
            continue;
        } else if(n == 0) continue;
        it.lock_.lock();
        for(std::list<mcc_tcp_handle_t>::iterator i = it.handles_.begin();i!=it.handles_.end();++i) {
            int s = i->handle;
            if(s == -1) continue;
            if(FD_ISSET(s,&readfds)) {
                it.lock_.unlock();
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);
                int h = ::accept(s,&addr,&addrlen);
                if(h == -1) {
                    logger.msg(ERROR, "Failed to accept connection request");
                    it.lock_.lock();
                } else {
                    it.lock_.lock();
                    bool rejected = false;
                    bool first_time = true;
                    while((it.max_executers_ > 0) &&
                          (it.executers_.size() >= (size_t) it.max_executers_)) {
                        if(it.max_executers_drop_) {
                            logger.msg(WARNING, "Too many connections - dropping new one");
                            ::shutdown(h,2);
                            ::close(h);
                            rejected = true;
                            break;
                        } else {
                            if(first_time)
                                logger.msg(WARNING, "Too many connections - waiting for old to close");
                            Glib::TimeVal etime;
                            etime.assign_current_time();
                            etime.add_milliseconds(10000); // 10 s
                            it.cond_.timed_wait(it.lock_,etime);
                            first_time = false;
                        };
                    };
                    if(!rejected) {
                      mcc_tcp_exec_t t(&it,h,i->timeout,i->no_delay);
                    };
                };
            };
        };
        it.lock_.unlock();
    };
    return;
}

class TCPSecAttr: public SecAttr {
 friend class MCC_TCP_Service;
 public:
  TCPSecAttr(const std::string& remote_ip, const std::string &remote_port, const std::string& local_ip, const std::string& local_port);
  virtual ~TCPSecAttr(void);
  virtual operator bool(void);
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::map< std::string, std::list<std::string> > getAll() const;
 protected:
  std::string local_ip_;
  std::string local_port_;
  std::string remote_ip_;
  std::string remote_port_;
  virtual bool equal(const SecAttr &b) const;
};

TCPSecAttr::TCPSecAttr(const std::string& remote_ip, const std::string &remote_port, const std::string& local_ip, const std::string& local_port) :
  local_ip_(local_ip), local_port_(local_port), remote_ip_(remote_ip), remote_port_(remote_port) {
}

TCPSecAttr::~TCPSecAttr(void) {
}

TCPSecAttr::operator bool(void) {
  return true;
}

std::string TCPSecAttr::get(const std::string& id) const {
  if(id == "LOCALIP") return local_ip_;
  if(id == "LOCALPORT") return local_port_;
  if(id == "REMOTEIP") return remote_ip_;
  if(id == "REMOTEPORT") return remote_port_;
  return "";
}

std::map<std::string, std::list<std::string> > TCPSecAttr::getAll() const {
  static char const * const allIds[] = {
    "LOCALIP",
    "LOCALPORT",
    "REMOTEIP",
    "REMOTEPORT",
    NULL
  };
  std::map<std::string, std::list<std::string> > all;
  for(char const * const * id = allIds; *id; ++id) {
    std::string idStr(*id);
    all[idStr] = SecAttr::getAll(idStr);
  }
  return all;
}

bool TCPSecAttr::equal(const SecAttr &b) const {
  try {
    const TCPSecAttr& a = (const TCPSecAttr&)b;
    if((!local_ip_.empty()) && (!a.local_ip_.empty()) && (local_ip_ != a.local_ip_)) return false;
    if((!local_port_.empty()) && (!a.local_port_.empty()) && (local_port_ != a.local_port_)) return false;
    if((!remote_ip_.empty()) && (!a.remote_ip_.empty()) && (remote_ip_ != a.remote_ip_)) return false;
    if((!remote_port_.empty()) && (!a.remote_port_.empty()) && (remote_port_ != a.remote_port_)) return false;
    return true;
  } catch(std::exception&) { };
  return false;
}

static void fill_arc_string_attribute(XMLNode object,std::string value,const char* id) {
  object=value;
  object.NewAttribute("Type")="string";
  object.NewAttribute("AttributeId")=id;
}

static void fill_xacml_string_attribute(XMLNode object,std::string value,const char* id) {
  object.NewChild("ra:AttributeValue")=value;
  object.NewAttribute("DataType")="xs:string";
  object.NewAttribute("AttributeId")=id;
}

#define TCP_SECATTR_REMOTE_NS "http://www.nordugrid.org/schemas/policy-arc/types/tcp/remoteendpoint"
#define TCP_SECATTR_LOCAL_NS "http://www.nordugrid.org/schemas/policy-arc/types/tcp/localendpoint"

bool TCPSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    if(!local_port_.empty()) {
      fill_arc_string_attribute(item.NewChild("ra:Resource"),local_ip_+":"+local_port_,TCP_SECATTR_LOCAL_NS);
    } else if(!local_ip_.empty()) {
      fill_arc_string_attribute(item.NewChild("ra:Resource"),local_ip_,TCP_SECATTR_LOCAL_NS);
    };
    if(!remote_port_.empty()) {
      fill_arc_string_attribute(item.NewChild("ra:Subject").NewChild("ra:SubjectAttribute"),remote_ip_+":"+remote_port_,TCP_SECATTR_REMOTE_NS);
    } else if(!remote_ip_.empty()) {
      fill_arc_string_attribute(item.NewChild("ra:Subject").NewChild("ra:SubjectAttribute"),remote_ip_,TCP_SECATTR_REMOTE_NS);
    };
    return true;
  } else if(format == XACML) {
    NS ns;
    ns["ra"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    val.Namespaces(ns); val.Name("ra:Request");
    if(!local_port_.empty()) {
      fill_xacml_string_attribute(val.NewChild("ra:Resource").NewChild("ra:Attribute"),local_ip_+":"+local_port_,TCP_SECATTR_LOCAL_NS);
    } else if(!local_ip_.empty()) {
      fill_xacml_string_attribute(val.NewChild("ra:Resource").NewChild("ra:Attribute"),local_ip_,TCP_SECATTR_LOCAL_NS);
    };
    if(!remote_port_.empty()) {
      fill_xacml_string_attribute(val.NewChild("ra:Subject").NewChild("ra:Attribute"),remote_ip_+":"+remote_port_,TCP_SECATTR_REMOTE_NS);
    } else if(!remote_ip_.empty()) {
      fill_xacml_string_attribute(val.NewChild("ra:Subject").NewChild("ra:Attribute"),remote_ip_,"http://www.nordugrid.org/schemas/policy-arc/types/tcp/remoteiendpoint");
    };
    return true;
  } else {
  };
  return false;
}

static bool get_host_port(struct sockaddr_storage *addr, std::string &host, std::string &port)
{
    char buf[INET6_ADDRSTRLEN];
    memset(buf,0,sizeof(buf));
    const char *ret = NULL;
    switch (addr->ss_family) {
        case AF_INET: {
            struct sockaddr_in *sin = (struct sockaddr_in *)addr;
            ret = inet_ntop(AF_INET, &(sin->sin_addr), buf, sizeof(buf)-1);
            if (ret != NULL) {
                port = tostring(ntohs(sin->sin_port));
            }
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr;
            if (!IN6_IS_ADDR_V4MAPPED(&(sin6->sin6_addr))) {
                ret = inet_ntop(AF_INET6, &(sin6->sin6_addr), buf, sizeof(buf)-1);
            } else {
                // ipv4 address mapped to ipv6 so resolve as ipv4 address
                struct sockaddr_in sin;
                memset(&sin, 0, sizeof(struct sockaddr_in));
                sin.sin_family = AF_INET;
                sin.sin_port = sin6->sin6_port;
                sin.sin_addr.s_addr = ((uint32_t *)&sin6->sin6_addr)[3];
                memcpy(addr, &sin, sizeof(struct sockaddr_in));
                ret = inet_ntop(AF_INET, &(sin.sin_addr), buf, sizeof(buf)-1);
            }
            if (ret != NULL) {
                port = tostring(ntohs(sin6->sin6_port));
            }
            break;
        }
        default:
            return false;
            break;
    }
    if (ret != NULL) {
        buf[sizeof(buf)-1] = 0;
        host = buf;
    } else {
        return false;
    }
    return true;
}

void MCC_TCP_Service::executer(void* arg) {
    MCC_TCP_Service& it = *(((mcc_tcp_exec_t*)arg)->obj);
    int s = ((mcc_tcp_exec_t*)arg)->handle;
    int no_delay = ((mcc_tcp_exec_t*)arg)->no_delay;
    int timeout = ((mcc_tcp_exec_t*)arg)->timeout;
    std::string host_attr,port_attr;
    std::string remotehost_attr,remoteport_attr;
    std::string endpoint_attr;
    // Extract useful attributes
    {
        struct sockaddr_storage addr;
        socklen_t addrlen;
        addrlen=sizeof(addr);
        if(getsockname(s, (struct sockaddr*)(&addr), &addrlen) == 0) {
            if (get_host_port(&addr, host_attr, port_attr) == true) {
                endpoint_attr = "://"+host_attr+":"+port_attr;
            }
        }
        if(getpeername(s, (struct sockaddr*)&addr, &addrlen) == 0) {
            get_host_port(&addr, remotehost_attr, remoteport_attr);
        }
        // SESSIONID
    };

    // Creating stream payload
    PayloadTCPSocket stream(s, timeout, logger);
    stream.NoDelay(no_delay);
    MessageContext context;
    MessageAuthContext auth_context;
    for(;;) {
        // TODO: Check state of socket here and leave immediately if not connected anymore.
        // Preparing Message objects for chain
        MessageAttributes attributes_in;
        MessageAttributes attributes_out;
        MessageAuth auth_in;
        MessageAuth auth_out;
        Message nextinmsg;
        Message nextoutmsg;
        nextinmsg.Payload(&stream);
        nextinmsg.Attributes(&attributes_in);
        nextinmsg.Attributes()->set("TCP:HOST",host_attr);
        nextinmsg.Attributes()->set("TCP:PORT",port_attr);
        nextinmsg.Attributes()->set("TCP:REMOTEHOST",remotehost_attr);
        nextinmsg.Attributes()->set("TCP:REMOTEPORT",remoteport_attr);
        nextinmsg.Attributes()->set("TCP:ENDPOINT",endpoint_attr);
        nextinmsg.Attributes()->set("ENDPOINT",endpoint_attr);
        nextinmsg.Context(&context);
        nextinmsg.Auth(&auth_in);
        TCPSecAttr* tattr = new TCPSecAttr(remotehost_attr, remoteport_attr, host_attr, port_attr);
        nextinmsg.Auth()->set("TCP",tattr);
        nextinmsg.AuthContext(&auth_context);
        nextoutmsg.Attributes(&attributes_out);
        nextoutmsg.Context(&context);
        nextoutmsg.Auth(&auth_out);
        nextoutmsg.AuthContext(&auth_context);
        if(!it.ProcessSecHandlers(nextinmsg,"incoming")) break;
        // Call next MCC
        MCCInterface* next = it.Next();
        if(!next) break;
        logger.msg(VERBOSE, "next chain element called");
        MCC_Status ret = next->process(nextinmsg,nextoutmsg);
        if(!it.ProcessSecHandlers(nextoutmsg,"outgoing")) {
          if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
          break;
        };
        // If nextoutmsg contains some useful payload send it here.
        // So far only buffer payload is supported
        // Extracting payload
        if(nextoutmsg.Payload()) {
            PayloadRawInterface* outpayload = NULL;
            try {
                outpayload = dynamic_cast<PayloadRawInterface*>(nextoutmsg.Payload());
            } catch(std::exception& e) { };
            if(!outpayload) {
                logger.msg(WARNING, "Only Raw Buffer payload is supported for output");
            } else {
                // Sending payload
                for(int n=0;;++n) {
                    char* buf = outpayload->Buffer(n);
                    if(!buf) break;
                    int bufsize = outpayload->BufferSize(n);
                    if(!(stream.Put(buf,bufsize))) {
                        logger.msg(ERROR, "Failed to send content of buffer");
                        break;
                    };
                };
            };
            delete nextoutmsg.Payload();
        };
        if(!ret) break;
    };
    it.lock_.lock();
    for(std::list<mcc_tcp_exec_t>::iterator e = it.executers_.begin();e != it.executers_.end();++e) {
        if((mcc_tcp_exec_t*)arg == &(*e)) {
            logger.msg(VERBOSE, "TCP executor is removed");
            if(s != e->handle)
              logger.msg(ERROR, "Sockets do not match on exit %i != %i",s,e->handle);
            it.executers_.erase(e);
            break;
        };
    };
    ::shutdown(s,2);
    ::close(s);
    it.cond_.signal();
    it.lock_.unlock();
    return;
}

MCC_Status MCC_TCP_Service::process(Message&,Message&) {
  // Service is not really processing messages because there
  // are no lower lelel MCCs in chain.
  return MCC_Status();
}

MCC_TCP_Client::MCC_TCP_Client(Config *cfg, PluginArgument* parg):MCC_TCP(cfg,parg),s_(NULL) {
    XMLNode c = (*cfg)["Connect"][0];
    if(!c) {
        logger.msg(ERROR,"No Connect element specified");
        return;
    };

    std::string port_s = c["Port"];
    if(port_s.empty()) {
        logger.msg(ERROR,"Missing Port in Connect element");
        return;
    };

    std::string host_s = c["Host"];
    if(host_s.empty()) {
        logger.msg(ERROR,"Missing Host in Connect element");
        return;
    };

    int port = atoi(port_s.c_str());

    std::string timeout_s = c["Timeout"];
    int timeout = 60;
    if (!timeout_s.empty()) {
        timeout = atoi(timeout_s.c_str());
    }
    s_ = new PayloadTCPSocket(host_s.c_str(),port,timeout,logger);
    if(!(*s_)) {
       // Connection error is reported in process()
    } else {
       std::string v = c["NoDelay"];
       s_->NoDelay((v == "true") || (v == "1"));
    };
}

MCC_TCP_Client::~MCC_TCP_Client(void) {
    if(s_) delete(s_);
}

MCC_Status MCC_TCP_Client::process(Message& inmsg,Message& outmsg) {
    // Accepted payload is Raw and Stream
    // Returned payload is Stream

    logger.msg(DEBUG, "TCP client process called");
    //outmsg.Attributes(inmsg.Attributes());
    //outmsg.Context(inmsg.Context());
    if(!s_) return MCC_Status(GENERIC_ERROR,"TCP","Not connected");
    if(!*s_) return MCC_Status(GENERIC_ERROR,"TCP",s_->GetError());
    // Extracting payload
    if(!inmsg.Payload()) return MCC_Status(GENERIC_ERROR);
    PayloadRawInterface* rinpayload = NULL;
    PayloadStreamInterface* sinpayload = NULL;
    try {
        rinpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    try {
        sinpayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if((!rinpayload) && (!sinpayload)) return MCC_Status(GENERIC_ERROR);
    if(!ProcessSecHandlers(inmsg,"outgoing")) return MCC_Status(GENERIC_ERROR,"TCP","Auth processing failed");
    // Sending payload
    if(rinpayload) {
        for(int n=0;;++n) {
            char* buf = rinpayload->Buffer(n);
            if(!buf) break;
            int bufsize = rinpayload->BufferSize(n);
            if(!(s_->Put(buf,bufsize))) {
                logger.msg(INFO, "Failed to send content of buffer");
                return MCC_Status(GENERIC_ERROR,"TCP",s_->GetError());
            };
        };
    } else {
        int size = -1;
        if(!sinpayload->Get(*s_,size)) {
        // Currently false may also mean that stream finihsed. 
        // Hence it can't be used to indicate real failure.
        //    logger.msg(INFO, "Failed to transfer content of stream");
        //    return MCC_Status(GENERIC_ERROR,"TCP",s_->GetError());
        };
    };
    std::string host_attr,port_attr;
    std::string remotehost_attr,remoteport_attr;
    std::string endpoint_attr;
    // Extract useful attributes
    {
      struct sockaddr_storage addr;
      socklen_t addrlen;
      addrlen=sizeof(addr);
      if (getsockname(s_->GetHandle(), (struct sockaddr*)&addr, &addrlen) == 0)
        get_host_port(&addr, host_attr, port_attr);
      addrlen=sizeof(addr);
      if (getpeername(s_->GetHandle(), (struct sockaddr*)&addr, &addrlen) == 0)
        if (get_host_port(&addr, remotehost_attr, remoteport_attr))
          endpoint_attr = "://"+remotehost_attr+":"+remoteport_attr;
    }
    outmsg.Payload(new PayloadTCPSocket(*s_));
    outmsg.Attributes()->set("TCP:HOST",host_attr);
    outmsg.Attributes()->set("TCP:PORT",port_attr);
    outmsg.Attributes()->set("TCP:REMOTEHOST",remotehost_attr);
    outmsg.Attributes()->set("TCP:REMOTEPORT",remoteport_attr);
    outmsg.Attributes()->set("TCP:ENDPOINT",endpoint_attr);
    outmsg.Attributes()->set("ENDPOINT",endpoint_attr);
    if(!ProcessSecHandlers(outmsg,"incoming")) return MCC_Status(GENERIC_ERROR,"TCP","Auth processing failed");
    return MCC_Status(STATUS_OK);
}

} // namespace ArcMCCTCP
