#ifndef __ARC_SUBMITTERARC1_H__
#define __ARC_SUBMITTERARC1_H__

#include <arc/client/Submitter.h>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>
#include <arc/URL.h>

namespace Arc { 

  class ChainContext;
  class Config;

  class SubmitterARC1
    : public Submitter {

  private:

    //! The configuration.
    /*! A configuration object containing information about how to set
      up this A-REX client.
     */
    Arc::Config* client_config;

    //! The loader.
    /*! A loader object that loads and connects the appropriate
      components according to the configuration object.
     */
    Arc::Loader* client_loader;

    Arc::ClientSOAP* client;

    //! The entry into the client message chain.
    /*! This is a pointer to the message chain components (MCC) where
      messages sent from this client enters the message chain.
     */
    Arc::MCC* client_entry;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    Arc::NS arex_ns;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the A-REX
      client are sent.
     */
    static Arc::Logger logger;
    
    Arc::MCCConfig mcc_cfg;
    
    SubmitterARC1(Config *cfg);
    ~SubmitterARC1();

  public:
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    std::pair<URL, URL> Submit(Arc::JobDescription& jobdesc);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC1_H__
