#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <unistd.h>

#ifdef WIN32
#include <arc/win32.h>
#include <winsock2.h>
typedef int socklen_t;
#define ErrNo WSAGetLastError()

#include <stdio.h>
// There is no inet_ntop on WIN32
inline const char *inet_ntop(int af, const void *__restrict src, char *__restrict dest, socklen_t size)
{
	// IPV6 not supported (yet?)
	if(AF_INET!=af)
	{
		printf("inet_ntop is only implemented for AF_INET address family on win32/msvc8");
		abort();
	}

	// Format address
	char *s=inet_ntoa(*reinterpret_cast<const in_addr*>(src));
	if(!s)
		return 0;

	// Copy to given buffer
	socklen_t len=(socklen_t)strlen(s);
	if(len>=size)
		return 0;
	return strncpy(dest, s, len);
}

#else // UNIX
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define ErrNo errno
#endif

#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "MCCTCP.h"

#define PROTO_NAME(ADDR) ((ADDR->ai_family==AF_INET6)?"IPv6":"IPv4")
Arc::Logger Arc::MCC_TCP::logger(Arc::MCC::logger,"TCP");

Arc::MCC_TCP::MCC_TCP(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Arc::MCC_TCP_Service((Arc::Config*)(*mccarg));
}

static Arc::Plugin* get_mcc_client(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Arc::MCC_TCP_Client((Arc::Config*)(*mccarg));
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "tcp.service", "HED:MCC", 0, &get_mcc_service },
    { "tcp.client",  "HED:MCC", 0, &get_mcc_client  },
    { NULL, NULL, 0, NULL }
};


namespace Arc {


MCC_TCP_Service::MCC_TCP_Service(Arc::Config *cfg):MCC_TCP(cfg) {
#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
    	logger.msg(Arc::ERROR, "Cannot initialize winsock library");
        return;
    }
#endif
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
            logger.msg(Arc::ERROR, "Missing Port in Listen element");
            continue;
        };
        std::string version_s = l["Version"];
        if(!version_s.empty()) {
            if(version_s == "4") { hint.ai_family = AF_INET; }
            else if(version_s == "6") { hint.ai_family = AF_INET6; }
            else {
                logger.msg(Arc::ERROR, "Version in Listen element can't be recognized");
                continue;
            };
        };
        int ret = getaddrinfo(NULL, port_s.c_str(), &hint, &info);
        if (ret != 0) {
            std::string err_str = gai_strerror(ret);
            logger.msg(ERROR, "Failed to obtain local address for port %s - %s", port_s, err_str);
            continue;
        };
        for(struct addrinfo *info_ = info;info_;info_=info_->ai_next) {
            logger.msg(Arc::DEBUG, "Trying to listen on TCP port %s(%s)", port_s, PROTO_NAME(info_));
            int s = ::socket(info_->ai_family,info_->ai_socktype,info_->ai_protocol);
            if(s == -1) {
                std::string e = StrError(errno);
	        logger.msg(Arc::ERROR, "Failed to create socket for for listening at TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                continue;
            };
            if(::bind(s,info->ai_addr,info->ai_addrlen) == -1) {
                std::string e = StrError(errno);
	        logger.msg(Arc::ERROR, "Failed to bind socket for TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                close(s);
                continue;
            };
            if(::listen(s,-1) == -1) {
                std::string e = StrError(errno);
	        logger.msg(Arc::WARNING, "Failed to listen at TCP port %s(%s): %s", port_s, PROTO_NAME(info_),e);
                close(s);
                continue;
            };
            handles_.push_back(s);
            logger.msg(Arc::INFO, "Listening on TCP port %s(%s)", port_s, PROTO_NAME(info_));
        };
        freeaddrinfo(info);
    };
    if(handles_.size() == 0) {
        logger.msg(Arc::ERROR, "No listening ports initiated");
        return;
    };
    if(!CreateThreadFunction(&listener,this)) {
        logger.msg(Arc::ERROR, "Failed to start thread for listening");
        for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();i=handles_.erase(i)) ::close(*i);
    };
}

MCC_TCP_Service::~MCC_TCP_Service(void) {
    //logger.msg(Arc::DEBUG, "TCP_Service destroy");
    lock_.lock();
    for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();++i) {
        ::close(*i); *i=-1;
    };
    for(std::list<mcc_tcp_exec_t>::iterator e = executers_.begin();e != executers_.end();++e) {
        ::close(e->handle); e->handle=-1;
    };
    // Wait for threads to exit
    while(executers_.size() > 0) {
        lock_.unlock(); sleep(1); lock_.lock();
    };
    while(handles_.size() > 0) {
        lock_.unlock(); sleep(1); lock_.lock();
    };
    lock_.unlock();
#ifdef WIN32
    WSACleanup();
#endif
}

MCC_TCP_Service::mcc_tcp_exec_t::mcc_tcp_exec_t(MCC_TCP_Service* o,int h):obj(o),handle(h) {
    static int local_id = 0;
    if(handle == -1) return;
    id=local_id++;
    // list is locked externally
    std::list<mcc_tcp_exec_t>::iterator e = o->executers_.insert(o->executers_.end(),*this); 
    if(!CreateThreadFunction(&MCC_TCP_Service::executer,&(*e))) {
        logger.msg(Arc::ERROR, "Failed to start thread for communication");
        ::shutdown(handle,2);
#ifdef WIN32
        ::closesocket(handle); handle=-1; o->executers_.erase(e);
#else
        ::close(handle);  handle=-1; o->executers_.erase(e);
#endif
    };
}

void MCC_TCP_Service::listener(void* arg) {
    MCC_TCP_Service& it = *((MCC_TCP_Service*)arg);
    for(;;) {
        int max_s = -1;
        fd_set readfds;
        FD_ZERO(&readfds);
        it.lock_.lock();
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
            int s = *i;
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
	        logger.msg(Arc::ERROR,
			"Failed while waiting for connection request");
                it.lock_.lock();
                for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
                    int s = *i;
                    ::close(s); 
                    i=it.handles_.erase(i);
                };
                it.lock_.unlock();
                return;
            };
            continue;
        } else if(n == 0) continue;
        it.lock_.lock();
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();++i) {
            int s = *i;
            if(s == -1) continue;
            if(FD_ISSET(s,&readfds)) {
                it.lock_.unlock();
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);
                int h = accept(s,&addr,&addrlen);
                if(h == -1) {
                    logger.msg(Arc::ERROR, "Failed to accept connection request");
                    it.lock_.lock();
                } else {
                    it.lock_.lock();
                    mcc_tcp_exec_t t(&it,h);
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
 protected:
  std::string local_ip_;
  std::string local_port_;
  std::string remote_ip_;
  std::string remote_port_;
  virtual bool equal(const SecAttr &b) const;
};

TCPSecAttr::TCPSecAttr(const std::string& remote_ip, const std::string &remote_port, const std::string& local_ip, const std::string& local_port) :
 remote_ip_(remote_ip), remote_port_(remote_port), local_ip_(local_ip), local_port_(local_port) {
}

TCPSecAttr::~TCPSecAttr(void) {
}

TCPSecAttr::operator bool(void) {
  return true;
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

static void fill_string_attribute(XMLNode object,std::string value,const char* id) {
  object=value;
  object.NewAttribute("Type")="string";
  object.NewAttribute("AttributeId")=id;
}

bool TCPSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    if(!local_port_.empty()) {
      fill_string_attribute(item.NewChild("ra:Resource"),local_ip_+":"+local_port_,"http://www.nordugrid.org/schemas/policy-arc/types/tcp/localendpoint");
    } else if(!local_ip_.empty()) {
      fill_string_attribute(item.NewChild("ra:Resource"),local_ip_,"http://www.nordugrid.org/schemas/policy-arc/types/tcp/localendpoint");
    };
    if(!remote_port_.empty()) {
      fill_string_attribute(item.NewChild("ra:Subject").NewChild("ra:SubjectAttribute"),remote_ip_+":"+remote_port_,"http://www.nordugrid.org/schemas/policy-arc/types/tcp/remoteendpoint");
    } else if(!remote_ip_.empty()) {
      fill_string_attribute(item.NewChild("ra:Subject").NewChild("ra:SubjectAttribute"),remote_ip_,"http://www.nordugrid.org/schemas/policy-arc/types/tcp/remoteiendpoint");
    };
    return true;
  } else {
  };
  return false;
}

static bool get_host_port(struct sockaddr_storage *addr, std::string &host, std::string &port)
{
    char buf[INET6_ADDRSTRLEN];
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
    int id = ((mcc_tcp_exec_t*)arg)->id;
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
    PayloadTCPSocket stream(s, logger);
    MessageAttributes attributes_in;
    MessageAttributes attributes_out;
    MessageAuth auth_in;
    MessageAuth auth_out;
    MessageContext context;
    MessageAuthContext auth_context;
    for(;;) {
        // TODO: Check state of socket here and leave immediately if not connected anymore.
        // Preparing Message objects for chain
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
        nextoutmsg.Attributes(&attributes_out);
        nextinmsg.Auth(&auth_in);
        TCPSecAttr* tattr = new TCPSecAttr(remotehost_attr, remoteport_attr, host_attr, port_attr);
        nextinmsg.Auth()->set("TCP",tattr);
        nextoutmsg.Auth(&auth_out);
        nextoutmsg.Context(&context);
        nextoutmsg.AuthContext(&auth_context);
        if(!it.ProcessSecHandlers(nextinmsg,"incoming")) break;
        // Call next MCC 
        MCCInterface* next = it.Next();
        if(!next) break;
        logger.msg(Arc::DEBUG, "next chain element called");
        MCC_Status ret = next->process(nextinmsg,nextoutmsg);
        if(!it.ProcessSecHandlers(nextoutmsg,"outgoing")) break;
        // If nextoutmsg contains some useful payload send it here.
        // So far only buffer payload is supported
        // Extracting payload
        if(nextoutmsg.Payload()) {
            PayloadRawInterface* outpayload = NULL;
            try {
                outpayload = dynamic_cast<PayloadRawInterface*>(nextoutmsg.Payload());
            } catch(std::exception& e) { };
            if(!outpayload) {
                logger.msg(Arc::WARNING, "Only Raw Buffer payload is supported for output");
            } else {
                // Sending payload
                for(int n=0;;++n) {
                    char* buf = outpayload->Buffer(n);
                    if(!buf) break;
                    int bufsize = outpayload->BufferSize(n);
                    if(!(stream.Put(buf,bufsize))) {
                        logger.msg(Arc::ERROR, "Failed to send content of buffer");
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
        if(id == e->id) {
            s=e->handle;
            it.executers_.erase(e);
            break;
        };
    };
    ::shutdown(s,2);
    ::close(s);
    it.lock_.unlock();
    return;
}

MCC_Status MCC_TCP_Service::process(Message&,Message&) {
  // Service is not really processing messages because there 
  // are no lower lelel MCCs in chain.
  return MCC_Status();
}

MCC_TCP_Client::MCC_TCP_Client(Arc::Config *cfg):MCC_TCP(cfg),s_(NULL) {
#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
    	logger.msg(Arc::ERROR, "Cannot initialize winsock library");
        return;
    }
#endif
    XMLNode c = (*cfg)["Connect"][0];
    if(!c) {
        logger.msg(Arc::ERROR,"No Connect element specified");
        return;
    };

    std::string port_s = c["Port"];
    if(port_s.empty()) {
        logger.msg(Arc::ERROR,"Missing Port in Connect element");
        return;
    };

    std::string host_s = c["Host"];
    if(host_s.empty()) {
        logger.msg(Arc::ERROR,"Missing Host in Connect element");
        return;
    };

    int port = atoi(port_s.c_str());

    s_ = new PayloadTCPSocket(host_s.c_str(),port,logger);
    if(!(*s_)) { delete s_; s_ = NULL; };
}

MCC_TCP_Client::~MCC_TCP_Client(void) {
    if(s_) delete(s_);
#ifdef WIN32
    WSACleanup();
#endif
}

MCC_Status MCC_TCP_Client::process(Message& inmsg,Message& outmsg) {
    // Accepted payload is Raw
    // Returned payload is Stream
    
    logger.msg(Arc::DEBUG, "client process called");
    //outmsg.Attributes(inmsg.Attributes());
    //outmsg.Context(inmsg.Context());
    if(!s_) return MCC_Status(Arc::GENERIC_ERROR);
    // Extracting payload
    if(!inmsg.Payload()) return MCC_Status(Arc::GENERIC_ERROR);
    PayloadRawInterface* inpayload = NULL;
    try {
        inpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) return MCC_Status(Arc::GENERIC_ERROR);
    if(!ProcessSecHandlers(inmsg,"outgoing")) return MCC_Status(Arc::GENERIC_ERROR);
    // Sending payload
    for(int n=0;;++n) {
        char* buf = inpayload->Buffer(n);
        if(!buf) break;
        int bufsize = inpayload->BufferSize(n);
        if(!(s_->Put(buf,bufsize))) {
            logger.msg(Arc::ERROR, "Failed to send content of buffer");
            return MCC_Status();
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
    if(!ProcessSecHandlers(outmsg,"incoming")) return MCC_Status(Arc::GENERIC_ERROR);
    return MCC_Status(Arc::STATUS_OK);
}
} // namespace ARC
