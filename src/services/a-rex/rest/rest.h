#ifndef __ARC_AREX_REST_H__
#define __ARC_AREX_REST_H__

#include <arc/message/Message.h>
#include <arc/loader/Plugin.h>
#include <arc/Logger.h>
#include <arc/ArcConfig.h>
#include "../grid-manager/conf/GMConfig.h"

#define HTTP_ERR_NOT_SUPPORTED (501)
#define HTTP_ERR_FORBIDDEN (403)

namespace ARex {


  class ARexRest {
   public:
    ARexRest(Arc::Config *cfg, Arc::PluginArgument *parg, GMConfig& config, ARex::DelegationStores& delegation_stores, unsigned int& all_jobs_count);
    virtual ~ARexRest(void);
    Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);

   private:
    class ProcessingContext {
     public:
      std::string subpath;
      std::string method;
      std::string processed;
      std::multimap<std::string,std::string> query;
      std::string operator[](char const * key) const;
      enum Version {
        Version_undefined = 0,
        Version_1_0 = 1,
        Version_1_1 = 2
      };
      Version version;
      ProcessingContext():version(Version_undefined) {};
    };

    Arc::Logger logger_;
    std::string uname_;
    std::string endpoint_;
    FileChunksList files_chunks_;

    ARex::GMConfig& config_;
    ARex::DelegationStores& delegation_stores_;
    unsigned int& all_jobs_count_;

    Arc::MCC_Status processVersions(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context);

    Arc::MCC_Status processGeneral(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context);

    Arc::MCC_Status processInfo(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context);
    Arc::MCC_Status processJobs(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context);
    Arc::MCC_Status processDelegations(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context);

    Arc::MCC_Status processDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,
                        std::string const & id);
    Arc::MCC_Status processJob(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,
                        std::string const & id);
    Arc::MCC_Status processJobSessionDir(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,
                        std::string const & id);
    Arc::MCC_Status processJobControlDir(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,
                        std::string const & id);


    Arc::MCC_Status processJobSub(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,
                        std::string const & id, std::string const & subResource);
    Arc::MCC_Status processJobSession(Arc::Message& inmsg,Arc::Message& outmsg,
                        ProcessingContext& context,std::string const & id);
    //Arc::MCC_Status processJobDelegations(Arc::Message& inmsg,Arc::Message& outmsg,
    //                    ProcessingContext& context,std::string const & id);
    //Arc::MCC_Status processJobDelegation(Arc::Message& inmsg,Arc::Message& outmsg,
    //                    ProcessingContext& context,std::string const & jobId,std::string const & delegId);
};

} // namespace ARex

#endif

