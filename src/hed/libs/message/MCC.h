#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include <list>
#include <map>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/security/SecHandler.h>
#include <arc/loader/Plugin.h>
#include "Message.h"
#include "MCC_Status.h"

namespace Arc {

  /// Interface for communication between MCC, Service and Plexer objects.
  /** The Interface consists of the method process() which is called by
      the previous MCC in the chain. For memory management policies please
      read the description of the Message class. */
  class MCCInterface: public Plugin {
  public:
    /** Method for processing of requests and responses.
       	This method is called by preceeding MCC in chain when a request
       	needs to be processed.
       	This method must call similar method of next MCC in chain unless
       	any failure happens. Result returned by call to next MCC should
       	be processed and passed back to previous MCC.
       	In case of failure this method is expected to generate valid
       	error response and return it back to previous MCC without calling
       	the next one.
     \param request The request that needs to be processed.
     \param response A Message object that will contain the response
       	of the request when the method returns.
     \return An object representing the status of the call.
     */
    virtual Arc::MCC_Status process(Arc::Message& request,
				    Arc::Message& response) = 0;
    virtual ~MCCInterface() {}
  };

  /// Message Chain Component - base class for every MCC plugin.
  /** This is partialy virtual class which defines interface and common
      functionality for every MCC plugin needed for managing of component
      in a chain. */
  class MCC
    : public Arc::MCCInterface {
  protected:
    /** Set of labeled "next" components.
       	Each implemented MCC must call process() method of
       	corresponding MCCInterface from this set in own process() method. */
    std::map<std::string, Arc::MCCInterface *> next_;
    Arc::MCCInterface *Next(const std::string& label = "");

    /** Set of labeled authentication and authorization handlers.
       	MCC calls sequence of handlers at specific point depending
       	on associated identifier. In most aces those are "in" and "out"
       	for incoming and outgoing messages correspondingly. */
    std::map<std::string, std::list<ArcSec::SecHandler *> > sechandlers_;

    /** Executes security handlers of specified queue.
       	Returns true if the message is authorized for further processing or if
       	there are no security handlers which implement authorization
       	functionality.
       	This is a convenience method and has to be called by the implemention
       	of the MCC. */
    bool ProcessSecHandlers(Arc::Message& message,
			    const std::string& label = "");

    /// A logger for MCCs.
    /** A logger intended to be the parent of loggers in the different
       	MCCs. */
    static Arc::Logger logger;

  public:
    /** Example contructor - MCC takes at least it's configuration subtree */
    MCC(Arc::Config *) {}

    virtual ~MCC() {}

    /** Add reference to next MCC in chain.
       	This method is called by Loader for every potentially labeled link to
       	next component which implements MCCInterface. If next is NULL
       	corresponding link is removed.  */
    virtual void Next(Arc::MCCInterface *next, const std::string& label = "");

    /** Add security components/handlers to this MCC.
       	Security handlers are stacked into a few queues with each queue
       	identified by its label. The queue labelled 'incoming' is executed for
       	every 'request' message after the message is processed by the MCC on
       	the service side and before processing on the client side. The queue
       	labelled 'outgoing' is run for response message before it is processed
       	by MCC algorithms on the service side and after processing on the
       	client side. Those labels are just a matter of agreement
       	and some MCCs may implement different queues executed at various
       	message processing steps. */
    virtual void AddSecHandler(Arc::Config *cfg,
			       ArcSec::SecHandler *sechandler,
			       const std::string& label = "");

    /** Removing all links. Useful for destroying chains. */
    virtual void Unlink();

    /** Dummy Message processing method. Just a placeholder. */
    virtual Arc::MCC_Status process(Arc::Message& /* request */,
				    Arc::Message& /* response */) {
      return MCC_Status();
    }
  };

  class MCCConfig
    : public BaseConfig {
  public:
    MCCConfig()
      : BaseConfig() {}
    virtual ~MCCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  class SecHandlerConfig {
  private:
    XMLNode cfg_;
  public:
    SecHandlerConfig(XMLNode cfg);
    virtual ~SecHandlerConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  #define MCCPluginKind ("HED:MCC")

  class ChainContext;

  class MCCPluginArgument: public PluginArgument {
   private:
    Config* config_;
    ChainContext* context_;
   public:
    MCCPluginArgument(Config* config,ChainContext* context):config_(config),context_(context) { };
    virtual ~MCCPluginArgument(void) { };
    operator Config* (void) { return config_; };
    operator ChainContext* (void) { return context_; };
  };

} // namespace Arc

#endif /* __ARC_MCC_H__ */
