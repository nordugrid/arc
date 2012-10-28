// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <algorithm>
#include <set>
#include <string>

#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/client/EntityRetriever.h>
#include <arc/client/JobDescription.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class URL;
  class UserConfig;

  class BrokerPluginArgument : public PluginArgument {
  public:
    BrokerPluginArgument(const UserConfig& uc) : uc(uc) {}
    ~BrokerPluginArgument() {}
    operator const UserConfig&() const { return uc; }
  private:
    const UserConfig& uc;
  };

  class BrokerPlugin : public Plugin {
  public:
    BrokerPlugin(BrokerPluginArgument* arg) : Plugin(arg), uc(*arg), j(NULL) {}
    virtual bool operator() (const ExecutionTarget&, const ExecutionTarget&) const;
    virtual bool match(const ExecutionTarget& et) const;
    virtual void set(const JobDescription& _j) const;
  protected:
    const UserConfig& uc;
    mutable const JobDescription* j;
    
    static Logger logger;
  };

  class BrokerPluginLoader : public Loader {
  public:
    BrokerPluginLoader();
    ~BrokerPluginLoader();
    BrokerPlugin* load(const UserConfig& uc, const std::string& name = "", bool keep_ownerskip = true);
    BrokerPlugin* load(const UserConfig& uc, const JobDescription& j, const std::string& name = "", bool keep_ownerskip = true);
    BrokerPlugin* copy(const BrokerPlugin* p, bool keep_ownerskip = true);

  private:
    BrokerPlugin* load(const UserConfig& uc, const JobDescription* j, const std::string& name, bool keep_ownerskip);
    
    std::list<BrokerPlugin*> plugins;
  };

  class Broker {
  public:
    Broker(const UserConfig& uc, const std::string& name = "");
    Broker(const UserConfig& uc, const JobDescription& j, const std::string& name = "");
    Broker(const Broker& b);
    ~Broker();
    
    Broker& operator=(const Broker& b);

    bool operator() (const ExecutionTarget& lhs, const ExecutionTarget& rhs) const;
    bool match(const ExecutionTarget& et) const;
    static bool genericMatch(const ExecutionTarget& et, const JobDescription& j, const Arc::UserConfig&);
    bool isValid(bool alsoCheckJobDescription = true) const;
    void set(const JobDescription& _j) const;
    const JobDescription& getJobDescription() const { return *j; }
    
  private:
    const UserConfig& uc;
    mutable const JobDescription* j;

    std::string proxyDN;
    std::string proxyIssuerCA;

    CountedPointer<BrokerPlugin> p;

    static BrokerPluginLoader l;

    static Logger logger;
  };

  class ExecutionTargetSorter : public EntityConsumer<ComputingServiceType> {
  public:
    ExecutionTargetSorter(const Broker& b, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) {}
    ExecutionTargetSorter(const Broker& b, const JobDescription& j, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { set(j); }
    ExecutionTargetSorter(const Broker& b, const std::list<ComputingServiceType>& csList, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { addEntities(csList); }
    ExecutionTargetSorter(const Broker& b, const JobDescription& j, const std::list<ComputingServiceType>& csList, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { set(j); addEntities(csList); }
    virtual ~ExecutionTargetSorter() {}

    void addEntity(const ExecutionTarget& et);
    void addEntity(const ComputingServiceType& cs);
    void addEntities(const std::list<ComputingServiceType>&);

    void reset() { current = targets.first.begin(); }
    bool next() { if (!endOfList()) { ++current; }; return !endOfList(); }
    bool endOfList() const { return current == targets.first.end(); }

    const ExecutionTarget& operator*() const { return *current; }
    const ExecutionTarget& getCurrentTarget() const { return *current; }
    const ExecutionTarget* operator->() const { return &*current; }

    const std::list<ExecutionTarget>& getMatchingTargets() const { return targets.first; }
    const std::list<ExecutionTarget>& getNonMatchingTargets() const { return targets.second; }

    void clear() { targets.first.clear(); targets.second.clear(); }

    void registerJobSubmission();

    void set(const Broker& newBroker) { b = &newBroker; sort(); }
    void set(const JobDescription& j) { b->set(j); sort(); }
    void setRejectEndpoints(const std::list<URL> newRejectEndpoints) { rejectEndpoints = newRejectEndpoints; }
    
  private:
    void sort();
    void insert(const ExecutionTarget& et);
    bool reject(const ExecutionTarget& et) const { return !rejectEndpoints.empty() && (std::find(rejectEndpoints.begin(), rejectEndpoints.end(), et.ComputingEndpoint->URLString) != rejectEndpoints.end() || std::find(rejectEndpoints.begin(), rejectEndpoints.end(), et.ComputingService->Cluster) != rejectEndpoints.end()); }  

    const Broker* b;
    std::list<URL> rejectEndpoints;
  
    std::pair< std::list<ExecutionTarget>, std::list<ExecutionTarget> > targets; // Map of ExecutionTargets. first: matching; second: unsuitable.
    std::list<ExecutionTarget>::iterator current;
    
    static Logger logger;
  };
  
  class CountedBroker : public CountedPointer<Broker> {
  public:
    CountedBroker(Broker* b) : CountedPointer<Broker>(b) {}
    CountedBroker(const CountedBroker& b) : CountedPointer<Broker>(b) {}
    bool operator()(const ExecutionTarget& lhs,const ExecutionTarget& rhs) const { return Ptr()->operator()(lhs, rhs); }
  };
  
  class ExecutionTargetSet : public std::set<ExecutionTarget, CountedBroker>, public EntityConsumer<ComputingServiceType> {
  private:
    template<typename InputIterator> void insert(InputIterator first, InputIterator last) { return; };
    
  public:
    ExecutionTargetSet(const Broker& b, const std::list<URL>& rejectEndpoints = std::list<URL>()) : std::set<ExecutionTarget, CountedBroker>(CountedBroker(new Broker(b))), rejectEndpoints(rejectEndpoints) {}
    ExecutionTargetSet(const Broker& b, const std::list<ComputingServiceType>& csList, const std::list<URL>& rejectEndpoints = std::list<URL>()) : std::set<ExecutionTarget, CountedBroker>(new Broker(b)), rejectEndpoints(rejectEndpoints) { addEntities(csList); }
    
    std::pair<ExecutionTargetSet::iterator, bool> insert(const ExecutionTarget& et) { logger.msg(VERBOSE, "ExecutionTarget %s added to ExecutionTargetSet", et.ComputingEndpoint->URLString); if (!reject(et) && key_comp()->match(et)) { return std::set<ExecutionTarget, CountedBroker>::insert(et); }; return std::pair<ExecutionTargetSet::iterator, bool>(end(), false); };
    ExecutionTargetSet::iterator insert(ExecutionTargetSet::iterator it, const ExecutionTarget& et) { if (!reject(et) && key_comp()->match(et)) { logger.msg(VERBOSE, "ExecutionTarget %s added to ExecutionTargetSet", et.ComputingEndpoint->URLString); return std::set<ExecutionTarget, CountedBroker>::insert(it, et); }; return end(); };
    void addEntity(const ComputingServiceType& cs) { cs.GetExecutionTargets(*this); }
    void addEntities(const std::list<ComputingServiceType>&);
    void set(const JobDescription& j) { clear(); key_comp()->set(j);}
  private:
    bool reject(const ExecutionTarget& et) const { return !rejectEndpoints.empty() && (std::find(rejectEndpoints.begin(), rejectEndpoints.end(), et.ComputingEndpoint->URLString) != rejectEndpoints.end() || std::find(rejectEndpoints.begin(), rejectEndpoints.end(), et.ComputingService->Cluster) != rejectEndpoints.end()); }
  
    std::list<URL> rejectEndpoints;
    
    static Logger logger;
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__
