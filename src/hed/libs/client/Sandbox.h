#ifndef __ARC_SANDBOX_H__
#define __ARC_SANDBOX_H__

#include <list>
#include <string>

#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>

namespace Arc {

  class Logger;
  
  class Sandbox {
  public:
    /* Add jobdescription of job to XMLNode which is
       stored in the sandbox
    */
    static bool Add(JobDescription& jobdesc, 
		   XMLNode& info);

    /* Calculate checksum of file */
    static std::string GetCksum(const std::string& file);

  private:
    static Logger logger;
    std::string sandboxfile;
    std::string conffile;
  };
} //namespace ARC

#endif // __ARC_SANDBOX_H__
