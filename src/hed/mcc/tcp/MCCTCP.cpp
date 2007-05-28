#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../../libs/message/PayloadStream.h"
#include "../../libs/message/PayloadRaw.h"
#include "../../libs/loader/Loader.h"
#include "../../libs/loader/MCCLoader.h"
#include "../../../libs/common/XMLNode.h"
#include "../../../libs/common/Thread.h"

#include "MCCTCP.h"


static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::MCC_TCP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::MCC_TCP_Client(cfg);
}

mcc_descriptor __arc_mcc_modules__[] = {
    { "tcp.service", 0, &get_mcc_service },
    { "tcp.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


MCC_TCP_Service::MCC_TCP_Service(Arc::Config *cfg):MCC(cfg) {
    pthread_mutex_init(&lock_,NULL);
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
        struct sockaddr_in myaddr;
        memset(&myaddr,0,sizeof(myaddr));
        myaddr.sin_family=AF_INET;
        myaddr.sin_port=htons(port);
        myaddr.sin_addr.s_addr=INADDR_ANY;
        if(bind(s,(struct sockaddr *)&myaddr,sizeof(myaddr)) == -1) {
            std::cerr<<"Warning: Failed to bind socket"<<std::endl;
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
    //pthread_t thread;
    //if(pthread_create(&thread,NULL,&listener,this) != 0) {
    if(!CreateThreadFunction(&listener,this)) {
        std::cerr<<"Error: Failed to start thread for listening"<<std::endl;
        for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();i=handles_.erase(i)) ::close(*i);
    };
}

MCC_TCP_Service::~MCC_TCP_Service(void) {
    pthread_mutex_lock(&lock_);
    for(std::list<int>::iterator i = handles_.begin();i!=handles_.end();++i) {
        ::close(*i); *i=-1;
    };
    for(std::list<mcc_tcp_exec_t>::iterator e = executers_.begin();e != executers_.end();++e) {
        ::close(e->handle); e->handle=-1;
    };
    pthread_mutex_unlock(&lock_);
    // Wait for threads to exit
    while(executers_.size() > 0) {
        pthread_mutex_unlock(&lock_); sleep(1); pthread_mutex_lock(&lock_);
    };
    while(handles_.size() > 0) {
        pthread_mutex_unlock(&lock_); sleep(1); pthread_mutex_lock(&lock_);
    };
    pthread_mutex_destroy(&lock_);
}

MCC_TCP_Service::mcc_tcp_exec_t::mcc_tcp_exec_t(MCC_TCP_Service* o,int h):obj(o),handle(h) {
    static int local_id = 0;
    //pthread_t thread;
    if(handle == -1) return;
    id=local_id++;
    //if(pthread_create(&thread,NULL,&MCC_TCP_Service::executer,this) != 0) {
    if(!CreateThreadFunction(&MCC_TCP_Service::executer,this)) {
        std::cerr<<"Error: Failed to start thread for communication"<<std::endl;
        ::close(handle);  handle=-1;
    };
}

void MCC_TCP_Service::listener(void* arg) {
    MCC_TCP_Service& it = *((MCC_TCP_Service*)arg);
    for(;;) {
        int max_s = -1;
        fd_set readfds;
        fd_set exfds;
        FD_ZERO(&readfds);
        pthread_mutex_lock(&it.lock_);
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
            int s = *i;
            if(s == -1) { i=it.handles_.erase(i); continue; };
            FD_SET(s,&readfds);
            if(s > max_s) max_s = s;
            ++i;
        };
        pthread_mutex_unlock(&it.lock_);
        if(max_s == -1) break;
        struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
        int n = select(max_s+1,&readfds,NULL,NULL,&tv);
        if(n < 0) {
            if(errno != EINTR) {
                std::cerr<<"Error: Failed while waiting for connection request"<<std::endl;
                pthread_mutex_lock(&it.lock_);
                for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();) {
                    int s = *i;
                    ::close(s); 
                    i=it.handles_.erase(i);
                };
                return;
            };
            continue;
        } else if(n == 0) continue;
        pthread_mutex_lock(&it.lock_);
        for(std::list<int>::iterator i = it.handles_.begin();i!=it.handles_.end();++i) {
            int s = *i;
            if(s == -1) continue;
            if(FD_ISSET(s,&readfds)) {
                pthread_mutex_unlock(&it.lock_);
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);
                int h = accept(s,&addr,&addrlen);
                if(h == -1) {
                    std::cerr<<"Error: Failed to accept connection request"<<std::endl;
                    pthread_mutex_lock(&it.lock_);
                } else {
                    mcc_tcp_exec_t t(&it,h);
                    pthread_mutex_lock(&it.lock_);
                    if(t) it.executers_.push_back(t);
                };
            };
        };
        pthread_mutex_unlock(&it.lock_);
    };
    return;
}

void MCC_TCP_Service::executer(void* arg) {
    MCC_TCP_Service& it = *(((mcc_tcp_exec_t*)arg)->obj);
    int s = ((mcc_tcp_exec_t*)arg)->handle;
    int id = ((mcc_tcp_exec_t*)arg)->id;
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
    pthread_mutex_lock(&it.lock_);
    for(std::list<mcc_tcp_exec_t>::iterator e = it.executers_.begin();e != it.executers_.end();++e) {
        if(id == e->id) {
            s=e->handle;
            it.executers_.erase(e);
            break;
        };
    };
    ::close(s);
    pthread_mutex_unlock(&it.lock_);
    return;
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

