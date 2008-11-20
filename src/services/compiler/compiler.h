#ifndef __ARC_COMPILER_H__
#define __ARC_COMPILER_H__

#include <map>
#include <arc/message/Service.h>
#include <arc/Logger.h>
#include <arc/security/PDP.h>

namespace Compiler {
    
/** This service need  in the server config a "compiler:scriptfile_url"  element.
       It is the scriptfile place, wherefrom it will be download the JSDL.*/
    
class Service_Compiler: public Arc::Service
{
    protected:
        std::string script_url_;	
        Arc::NS ns_;
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
	static Arc::Logger logger;

    public:
        /** Constructor accepts configuration describing content of scriptfile_url */
        Service_Compiler(Arc::Config *cfg);
        virtual ~Service_Compiler(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
        std::string Get_Script_Url() {return script_url_;}	
};

} // namespace Compiler

#endif /* __ARC_COMPILER_H__ */
