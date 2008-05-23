// apstat.cpp

#include <iostream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arc/Logger.h>
#include <arc/misc/ClientTool.h>
#include <arc/delegation/DelegationInterface.h>

class APProxyTool: public Arc::ClientTool {
 public:
  std::string key_path;
  std::string cert_path;
  std::string proxy_path;
  std::map<std::string,std::string> constraints;
  APProxyTool(int argc,char* argv[]):Arc::ClientTool("apinfo") {
    ProcessOptions(argc,argv,"P:K:C:c:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"approxy [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-c constraints]"<<std::endl;
    std::cout<<"\tPossible debug levels are VERBOSE, DEBUG, INFO, WARNING, ERROR and FATAL"<<std::endl;
    std::cout<<"\tSupported constraints are:"<<std::endl;
    std::cout<<"\t\tvalidityStart=time"<<std::endl;
    std::cout<<"\t\tvalidityEnd=time"<<std::endl;
    std::cout<<"\t\tvalidityPeriod=time"<<std::endl;
    std::cout<<"\t\tproxyPolicy=policy content"<<std::endl;
  };
  virtual bool ProcessOption(char option,char* option_arg) {
    switch(option) {
      case 'P': proxy_path=option_arg;; break;
      case 'K': key_path=option_arg; break;
      case 'C': cert_path=option_arg; break;
      case 'c': {
        const char* p = strchr(option_arg,'=');
        if(!p) p=option_arg+strlen(option_arg);
        constraints[std::string(option_arg,p-option_arg)]=p+1;
      }; break;
      default: {
        std::cerr<<"Error processing option: "<<(char)option<<std::endl;
        PrintHelp();
        return false;
      };
    };
    return true;
  };
};

static Arc::Logger& logger = Arc::Logger::rootLogger;

static void tls_process_error(void) {
   unsigned long err;
   err = ERR_get_error();
   if (err != 0)
   {
     logger.msg(Arc::ERROR, "OpenSSL Error -- %s", ERR_error_string(err, NULL));
     logger.msg(Arc::ERROR, "Library  : %s", ERR_lib_error_string(err));
     logger.msg(Arc::ERROR, "Function : %s", ERR_func_error_string(err));
     logger.msg(Arc::ERROR, "Reason   : %s", ERR_reason_error_string(err));
   }
   return;
}

int main(int argc, char* argv[]){
  SSL_load_error_strings();
  SSL_library_init();
  APProxyTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if ((argc-tool.FirstOption())!=0)
      throw std::invalid_argument("Wrong number of arguments!");
    if(tool.key_path.empty()) {
      char* s = getenv("X509_USER_KEY");
      if((s == NULL) || (s[0] == 0)) {
        throw(std::runtime_error(std::string("Missing path to private key")));
      };
      tool.key_path=s;
    };
    if(tool.cert_path.empty()) {
      char* s = getenv("X509_USER_CERT");
      if((s == NULL) || (s[0] == 0)) {
        throw(std::runtime_error(std::string("Missing path to certificate")));
      };
      tool.cert_path=s;
    };
    if(tool.proxy_path.empty()) {
      char* s = getenv("X509_USER_PROXY");
      if((s == NULL) || (s[0] == 0)) {
        throw(std::runtime_error(std::string("Missing path to proxy")));
      };
      tool.proxy_path=s;
    };
    struct termios to;
    tcgetattr(STDIN_FILENO,&to);
    to.c_lflag &= ~ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
    Arc::DelegationProvider provider(tool.cert_path,tool.key_path,&std::cin);
    to.c_lflag |= ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
    if(!provider) {
      throw(std::runtime_error(std::string("Failed to acquire credentials")));
    };
    Arc::DelegationConsumer consumer;
    if(!consumer) {
      throw(std::runtime_error(std::string("Failed to generate new private key")));
    };
    std::string request;
    if(!consumer.Request(request)) {
      throw(std::runtime_error(std::string("Failed to generate certificate request")));
    };
    std::string proxy = provider.Delegate(request,tool.constraints);
    if(!consumer.Acquire(proxy)) {
      throw(std::runtime_error(std::string("Failed to generate proxy")));
    };
    int f = ::open(tool.proxy_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(f == -1) {
      throw(std::runtime_error(std::string("Failed to open proxy file ")+tool.proxy_path));
    };
    if(::write(f,proxy.c_str(),proxy.length()) != proxy.length()) {
      throw(std::runtime_error(std::string("Failed to write into proxy file ")+tool.proxy_path));
    };
    ::close(f);
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    tls_process_error();
    return EXIT_FAILURE;
  }
}
