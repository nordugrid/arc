#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include <cstdlib>
#include <iostream>
#include <fstream>

#include <arc/Logger.h>

#include "client.h"

#define olog (std::cerr)
#define odlog(LEVEL) (std::cerr)

// TODO: Run multiple clients in parallel

using namespace ARex;

int logger(const char* url,char const * const * dirs,int num,time_t ex_period = 0);
 
int main(int argc,char* argv[]) {
  char* url = NULL;

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  opterr=0;
  time_t ex_period = 0;
  int n;
  const char* basename = strrchr(argv[0],'/');
  if(basename == NULL) basename=argv[0];
  while((n=getopt(argc,argv,":hu:d:E:")) != -1) {
    switch(n) {
      case ':': { olog<<"Missing argument\n"; return 1; };
      case '?': { olog<<"Unrecognized option\n"; return 1; };
      case 'h': {
        std::cout<<"logger [-h] [-d level] [-u url] [-E expiration_period_days] control_dir ..."<<std::endl;
         return 0;
      };
      case 'u': {
        url=optarg;
      }; break;
      case 'd': {
        char* p;
        int i = strtol(optarg,&p,10);
        if(((*p) != 0) || (i<0)) {
          olog<<"Improper debug level '"<<optarg<<"'"<<std::endl;
          exit(1);
        };
//        LogTime::Level(NotifyLevel(FATAL+i));
      }; break;
      case 'E': {
        char* p;
        int i = strtol(optarg,&p,10);
        if(((*p) != 0) || (i<=0)) {
          olog<<"Improper expiration period '"<<optarg<<"'"<<std::endl;
          exit(1);
        };
        ex_period=i; ex_period*=(60*60*24);
      }; break;
      default: { olog<<"Options processing error\n"; return 1; };
    };
  };
  return logger(url,argv+optind,argc-optind,ex_period);
}

int logger(const char* url,char const * const * dirs,int num,time_t ex_period) {
  int ret = 0;
  LoggerClient client;
  for(int n = 0;dirs[n] && n<num;n++) {
    std::string logdir(dirs[n]); logdir+="/logs/";
    std::string logger_url(url?url:"");
    struct dirent file_;
    struct dirent *file;
    odlog(INFO)<<"Processing directory: "<<logdir<<std::endl;
    DIR *dir=opendir(logdir.c_str());
    if(dir == NULL) continue;
    for(;;) {
      readdir_r(dir,&file_,&file);
      if(file == NULL) break;
      std::string logfile = logdir+file->d_name;
      struct stat st;
      if(stat(logfile.c_str(),&st) != 0) continue;
      if(!S_ISREG(st.st_mode)) continue;
      odlog(INFO)<<"Processing file: "<<logfile<<std::endl;
      bool result = true;
      /* read file */
      char buf[4096];
      std::ifstream f(logfile.c_str());
      if(!f.is_open() ) continue; /* can't open file */
      JobRecord j(f);
      f.close();
      if(!j) {
        result=false;
        odlog(ERROR)<<"Removing unreadable job information in "<<logfile<<std::endl;
        unlink(logfile.c_str());
      } else {
        if(j.url.length()) logger_url=j.url;
        if(logger_url.length() == 0) {
          odlog(DEBUG)<<"No service URL provided"<<std::endl;
          result = false;
        } else {
          std::list<JobRecord> recs;
          recs.push_back(j);
          odlog(DEBUG)<<"Reporting to: "<<logger_url<<std::endl;
	  odlog(DEBUG)<<"Reporting about: "<<(std::string)(j["globaljobid"])<<std::endl;
//          result = client.Report(logger_url.c_str(),recs);
          result = client.ReportV2(logger_url.c_str(),recs);
        };
        if(result) {
          odlog(INFO)<<"Passed information about job "<<j["globaljobid"]<<std::endl;
          unlink(logfile.c_str());
        } else {
          odlog(ERROR)<<"Failed to pass information about job "<<
                  (std::string)(j["globaljobid"])<<std::endl;
          ret=-1;
          // Check creation time and remove it if really too old
          if((ex_period) && 
             (((unsigned int)(time(NULL)-st.st_mtime)) > ex_period)) {
            odlog(ERROR)<<"Removing outdated information about job "<<
                    (std::string)(j["globaljobid"])<<std::endl;
            unlink(logfile.c_str());
          };
        };
      };
    };
    closedir(dir);
  };
  return ret;
}

