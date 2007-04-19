#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../../PayloadStream.h"
#include "../../PayloadRaw.h"
#include "../../libs/loader/Loader.h"
#include "../../libs/loader/MCCLoader.h"
#include "../../../libs/common/XMLNode.h"

#include "MCCTCP.h"


static Arc::MCC* get_mcc_service(Arc::Config *cfg) {
    return new Arc::MCC_TCP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg) {
    return new Arc::MCC_TCP_Client(cfg);
}

mcc_descriptor __arc_mcc_modules__[] = {
    { "tcp.service", 0, &get_mcc_service },
    { "tcp.client", 0, &get_mcc_client }
};

using namespace Arc;


MCC_TCP_Service::MCC_TCP_Service(Arc::Config *cfg):MCC(cfg) {
    for(int i = 0;;++i) {
        XMLNode l = (*cfg)["Listen"][i];
        if(!l) break;
        std::string port_s = l["Port"];
        if(port_s.empty()) {
            std::cerr<<"Warning: Missing Port in Listen element"<<std::endl;
            continue;
        };
        int port = atoi(port_s.c_str());
        int s = ::socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(s == -1) {
            std::cerr<<"Warning: Failed to create socket"<<std::endl;
            continue;
        };
        if(::listen(s,-1) == -1) {
            std::cerr<<"Warning: Failed to listen on socket"<<std::endl;
            continue;
        };
        handles_.push_back(s);
    };
    if(handles_.size() == 0) {
        std::cerr<<"Error: No listening ports configured"<<std::endl;
        return;
    };
    pthread_t thread;
    if(pthread_create(&thread,NULL,&listener,this) != 0) {
        std::cerr<<"Error: Failed to start thread for listening"<<std::endl;
        for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();i=handles_.erase(i)) ::close(*i);
    };
}

MCC_TCP_Service::~MCC_TCP_Service(void) {
    for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();++i) ::close(*i);
    // TODO: Wait for thread to exit
}

class mcc_tcp_exec_t {
    public:
        MCC_TCP_Service* obj;
        int handle;
        mcc_tcp_exec_t(MCC_TCP_Service* o,int h):obj(o),handle(h) { };
};

void* MCC_TCP_Service::listener(void* arg) {
    MCC_TCP_Service& it = *((MCC_TCP_Service*)arg);
    for(;;) {
        int max_s = 0;
        fd_set readfds;
        FD_ZERO(&readfds);
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();++i) {
            int s = *i;
            FD_SET(s,&readfds);
            if(s > max_s) max_s = s;
        };
        int n = select(max_s+1,&readfds,NULL,NULL,NULL);
        if(n < 0) {
            if(errno != EINTR) {
                std::cerr<<"Error: Failed while waiting for connection request"<<std::endl;
                return NULL;
            };
            continue;
        } else if(n == 0) continue;
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();++i) {
            int s = *i;
            if(FD_ISSET(s,&readfds)) {
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);
                int h = accept(s,&addr,&addrlen);
                if(h == -1) {
                  std::cerr<<"Error: Failed to accept connection request"<<std::endl;
                } else {
                    pthread_t thread;
                    mcc_tcp_exec_t* t = new mcc_tcp_exec_t(&it,h);
                    if(pthread_create(&thread,NULL,&executer,t) != 0) {
                        std::cerr<<"Error: Failed to start thread for communication"<<std::endl;
                        ::close(h);
                    };
                };
            };
        };
    };
    return NULL;
}

void* MCC_TCP_Service::executer(void* arg) {
    MCC_TCP_Service& it = *(((mcc_tcp_exec_t*)arg)->obj);
    int s = ((mcc_tcp_exec_t*)arg)->handle;
    // Creating stream payload
    PayloadStream stream(s);
    for(;;) {
        // Preparing Message objects for chain
        Message nextinmsg;
        Message nextoutmsg;
        nextinmsg.Payload(&stream);
        // Call next MCC 
        MCCInterface* next = it.Next();
        if(!next) break;
        MCC_Status ret = next->process(nextinmsg,nextoutmsg);
        if(!ret) break;
    };
    return NULL;
}

MCC_Status MCC_TCP_Service::process(Message& inmsg,Message& outmsg) {
  return MCC_Status(-1);
}

MCC_TCP_Client::MCC_TCP_Client(Arc::Config *cfg):MCC(cfg),s_(NULL) {
    XMLNode c = (*cfg)["Connect"][0];
    if(!c) {
        std::cerr<<"Error: No Connect element specified"<<std::endl;
        return;
    };

    std::string port_s = c["Port"];
    if(port_s.empty()) {
        std::cerr<<"Error: Missing Port in Connect element"<<std::endl;
        return;
    };

    std::string host_s = c["Host"];
    if(host_s.empty()) {
        std::cerr<<"Error: Missing Host in Connect element"<<std::endl;
        return;
    };

    int port = atoi(port_s.c_str());

    s_ = new PayloadTCPSocket(host_s.c_str(),port);
    if(!(*s_)) { delete s_; s_ = NULL; };
}

MCC_TCP_Client::~MCC_TCP_Client(void) {
    if(s_) delete(s_);
}

MCC_Status MCC_TCP_Client::process(Message& inmsg,Message& outmsg) {
    // Accepted payload is Raw
    // Returned payload is Stream

    if(!s_) return MCC_Status(-1);
    // Extracting payload
    if(!inmsg.Payload()) return MCC_Status(-1);
    PayloadRawInterface* inpayload = NULL;
    try {
        inpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) return MCC_Status(-1);
    // Sending payload
    for(int n=0;;++n) {
        char* buf = inpayload->Buffer(n);
        if(!buf) break;
        int bufsize = inpayload->BufferSize(n);
        if(!(s_->Put(buf,bufsize))) {
            std::cerr<<"Error: Failed to send content of buffer"<<std::endl;
            return MCC_Status(-1);
        };
    };
    outmsg.Payload(new PayloadStream(*s_));
    return MCC_Status();
}

