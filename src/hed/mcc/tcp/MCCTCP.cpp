#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#define ErrNo errno
#endif

#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/loader/Loader.h>
#include <arc/loader/MCCLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "MCCTCP.h"

Arc::Logger Arc::MCC_TCP::logger(Arc::MCC::logger,"TCP");

Arc::MCC_TCP::MCC_TCP(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_TCP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_TCP_Client(cfg);
}

mcc_descriptors ARC_MCC_LOADER = {
    { "tcp.service", 0, &get_mcc_service },
    { "tcp.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


MCC_TCP_Service::MCC_TCP_Service(Arc::Config *cfg):MCC_TCP(cfg) {
#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
    	logger.msg(Arc::ERROR, "Cannot initialize winsock library");
        return;
    }
#endif
    for(int i = 0;;++i) {
        XMLNode l = (*cfg)["Listen"][i];
        if(!l) break;
        std::string port_s = l["Port"];
        if(port_s.empty()) {
            logger.msg(Arc::WARNING, "Missing Port in Listen element");
            continue;
        };
        int port = atoi(port_s.c_str());
        int s = ::socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(s == -1) {
	    logger.msg(Arc::WARNING, "Failed to create socket");
            continue;
        };
        struct sockaddr_in myaddr;
        memset(&myaddr,0,sizeof(myaddr));
        myaddr.sin_family=AF_INET;
        myaddr.sin_port=htons(port);
        myaddr.sin_addr.s_addr=INADDR_ANY;
        if(bind(s,(struct sockaddr *)&myaddr,sizeof(myaddr)) == -1) {
	    logger.msg(Arc::WARNING, "Failed to bind socket");
            continue;
        };
        if(::listen(s,-1) == -1) {
	    logger.msg(Arc::WARNING, "Failed to listen on socket");
            continue;
        };
        handles_.push_back(s);
    };
    if(handles_.size() == 0) {
        logger.msg(Arc::ERROR, "No listening ports configured");
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

void MCC_TCP_Service::executer(void* arg) {
    MCC_TCP_Service& it = *(((mcc_tcp_exec_t*)arg)->obj);
    int s = ((mcc_tcp_exec_t*)arg)->handle;
    int id = ((mcc_tcp_exec_t*)arg)->id;
    std::string host_attr,port_attr;
    std::string remotehost_attr,remoteport_attr;
    std::string endpoint_attr;
    // Extract useful attributes
    {
        struct sockaddr_in addr;
        socklen_t addrlen;
        addrlen=sizeof(addr);
        if(getsockname(s,(sockaddr*)(&addr),&addrlen) == 0) {
            char buf[256];
            if(inet_ntop(AF_INET,&addr.sin_addr,buf,sizeof(buf)-1) != NULL) {
                buf[sizeof(buf)-1]=0;
                host_attr=buf;
                port_attr=tostring(ntohs(addr.sin_port));
                endpoint_attr = "://"+host_attr+":"+port_attr;
            };
        };
        if(getpeername(s,(sockaddr*)(&addr),&addrlen) == 0) {
            char buf[256];
            if(inet_ntop(AF_INET,&addr.sin_addr,buf,sizeof(buf)-1) != NULL) {
                buf[sizeof(buf)-1]=0;
                remotehost_attr=buf;
                remoteport_attr=tostring(ntohs(addr.sin_port));
            };
        };
        // SESSIONID
    };
    // Creating stream payload
    PayloadStream stream(s);
    MessageAttributes attributes_in;
    MessageAttributes attributes_out;
    MessageContext context;
    for(;;) {
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
        nextoutmsg.Context(&context);
        if(!it.ProcessSecHandlers(nextinmsg,"incoming")) break;
        // Call next MCC 
        MCCInterface* next = it.Next();
        if(!next) break;
        logger.msg(Arc::DEBUG, "next chain element called");
        MCC_Status ret = next->process(nextinmsg,nextoutmsg);
        if(!it.ProcessSecHandlers(nextoutmsg,"outgoing")) break;
        // TODO: if nextoutmsg contains some useful payload send it here.
        if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
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
    ::close(s);
    it.lock_.unlock();
    return;
}

MCC_Status MCC_TCP_Service::process(Message&,Message&) {
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
    if(!ProcessSecHandlers(inmsg,"incoming")) return MCC_Status(Arc::GENERIC_ERROR);
    // Sending payload
    for(int n=0;;++n) {
        char* buf = inpayload->Buffer(n);
        if(!buf) break;
        logger.msg(Arc::DEBUG, "tcp: (%d, %d) %s", n, strlen(buf), buf);
        int bufsize = inpayload->BufferSize(n);
        if(!(s_->Put(buf,bufsize))) {
            logger.msg(Arc::ERROR, "Failed to send content of buffer");
            return MCC_Status();
        };
    };
    outmsg.Payload(new PayloadStream(*s_));
    if(!ProcessSecHandlers(outmsg,"outgoing")) return MCC_Status(Arc::GENERIC_ERROR);
    return MCC_Status(Arc::STATUS_OK);
}

