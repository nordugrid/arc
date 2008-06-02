#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/OptionParser.h>

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

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options("", "",
			    istring("Supported constraints are:\n"
				    "  validityStart=time\n"
				    "  validityEnd=time\n"
				    "  validityPeriod=time\n"
				    "  proxyPolicy=policy content\n"
                                    "  proxyPolicyFile=policy file"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to proxy file"),
		    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "certifcate", istring("path to certificate file"),
		    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to private key file"),
		    istring("path"), key_path);

  std::list<std::string> constraintlist;
  options.AddOption('c', "constraint", istring("proxy constraints"),
		    istring("string"), constraintlist);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (version) {
    std::cout << Arc::IString("%s version %s", "approxy", VERSION) << std::endl;
    return 0;
  }

  std::map<std::string, std::string> constraints;
  for (std::list<std::string>::iterator it = constraintlist.begin();
       it != constraintlist.end(); it++) {
    std::string::size_type pos = it->find('=');
    if (pos != std::string::npos)
      constraints[it->substr(0, pos)] = it->substr(pos + 1);
    else
      constraints[*it] = "";
  }

  SSL_load_error_strings();
  SSL_library_init();

  try{
    if (params.size()!=0)
      throw std::invalid_argument("Wrong number of arguments!");
    if(key_path.empty()) {
      char* s = getenv("X509_USER_KEY");
      if((s == NULL) || (s[0] == 0)) {
        throw std::runtime_error("Missing path to private key");
      };
      key_path=s;
    };
    if(cert_path.empty()) {
      char* s = getenv("X509_USER_CERT");
      if((s == NULL) || (s[0] == 0)) {
        throw std::runtime_error("Missing path to certificate");
      };
      cert_path=s;
    };
    if(proxy_path.empty()) {
      char* s = getenv("X509_USER_PROXY");
      if((s == NULL) || (s[0] == 0)) {
        throw std::runtime_error("Missing path to proxy");
      };
      proxy_path=s;
    };
    struct termios to;
    tcgetattr(STDIN_FILENO,&to);
    to.c_lflag &= ~ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
    Arc::DelegationProvider provider(cert_path,key_path,&std::cin);
    to.c_lflag |= ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
    if(!provider) {
      throw std::runtime_error("Failed to acquire credentials");
    };
    Arc::DelegationConsumer consumer;
    if(!consumer) {
      throw std::runtime_error("Failed to generate new private key");
    };
    std::string request;
    if(!consumer.Request(request)) {
      throw std::runtime_error("Failed to generate certificate request");
    };
    std::string proxy = provider.Delegate(request,constraints);
    if(!consumer.Acquire(proxy)) {
      throw std::runtime_error("Failed to generate proxy");
    };
    int f = ::open(proxy_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(f == -1) {
      throw std::runtime_error("Failed to open proxy file " + proxy_path);
    };
    if(::write(f,proxy.c_str(),proxy.length()) != proxy.length()) {
      throw std::runtime_error("Failed to write into proxy file " + proxy_path);
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
