// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <algorithm>
#include <set>
#include <string>

#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/BrokerPlugin.h>

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class URL;
  class UserConfig;

  /// A Broker filters and ranks acceptable targets for job submission.
  /**
   * This class is the high-level interface to brokers. It takes care of
   * loading at runtime the specific BrokerPlugin type which matches and ranks
   * ExecutionTargets according to specific criteria, for example queue length
   * or CPU benchmark. The type of BrokerPlugin to use is specified in the
   * constructor.
   *
   * The usual workflow is to call set() for the Broker to obtain the
   * parameters or constraints from the job that it is interested in, then
   * match() for each ExecutionTarget to filter targets. operator() can then be
   * used to sort the targets and is equivalent to
   * ExecutionTarget.operator<(ExecutionTarget).
   *
   * ExecutionTargetSorter can be used as a wrapper around Broker to avoid
   * calling Broker directly.
   *
   * \ingroup compute
   * \headerfile Broker.h arc/compute/Broker.h 
   */
  class Broker {
  public:
    /// Construct a new broker and load the BrokerPlugin of the given type.
    /**
     * \param uc UserConfig, passed to the BrokerPlugin.
     * \param name Name of the BrokerPlugin type to use. If empty then targets
     * are matched using genericMatch() but are not sorted.
     */
    Broker(const UserConfig& uc, const std::string& name = "");
    /// Construct a new broker of the given type and use the given JobDescription.
    /**
     * \param uc UserConfig, passed to the BrokerPlugin.
     * \param j set(j) is called from this constructor.
     * \param name Name of the BrokerPlugin type to use. If empty then targets
     * are matched using genericMatch() but are not sorted.
     */
    Broker(const UserConfig& uc, const JobDescription& j, const std::string& name = "");
    /// Copy constructor. BrokerPlugin copying is handled automatically.
    Broker(const Broker& b);
    /// Destructor. BrokerPlugin unloading is handled automatically.
    ~Broker();
    /// Assignment operator. BrokerPlugin copying is handled automatically.
    Broker& operator=(const Broker& b);

    /// Used to sort targets. Returns true if lhs<rhs according to BrokerPlugin.
    bool operator() (const ExecutionTarget& lhs, const ExecutionTarget& rhs) const;
    /// Returns true if the ExecutionTarget is allowed by BrokerPlugin.
    bool match(const ExecutionTarget& et) const;
    /// Perform a match between the given target and job.
    /**
     * This method is generally called by BrokerPlugins at the start of match()
     * to check that a target matches general attributes of the job such as CPU
     * and memory limits. The BrokerPlugin then does further matching depending
     * on its own criteria.
     * \return True if target matches job description.
     */
    static bool genericMatch(const ExecutionTarget& et, const JobDescription& j, const Arc::UserConfig&);
    /// Returns true if the BrokerPlugin loaded by this Broker is valid.
    /**
     * \param alsoCheckJobDescription Also check if JobDescription is valid.
     */
    bool isValid(bool alsoCheckJobDescription = true) const;
    /// Set the JobDescription to use during brokering.
    void set(const JobDescription& _j) const;
    /// Get the JobDescription set by set().
    const JobDescription& getJobDescription() const { return *j; }
    
  private:
    const UserConfig& uc;
    mutable const JobDescription* j;

    std::string proxyDN;
    std::string proxyIssuerCA;

    CountedPointer<BrokerPlugin> p;

    static BrokerPluginLoader& getLoader();

    static Logger logger;
  };

  /// Wrapper around Broker functionality.
  /**
   * This class can be used instead of calling Broker methods directly. It
   * automatically takes care of matching and sorting ExecutionTargets. It can
   * be thought of as an iterator over the list of sorted targets and supports
   * some iterator-style methods such as next(), operator-> and operator*.
   * \ingroup compute
   * \headerfile Broker.h arc/compute/Broker.h 
   */
  class ExecutionTargetSorter : public EntityConsumer<ComputingServiceType> {
  public:
    /// Basic constructor.
    ExecutionTargetSorter(const Broker& b, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) {}
    /// Constructor passing JobDescription.
    ExecutionTargetSorter(const Broker& b, const JobDescription& j, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { set(j); }
    /// Constructor passing list of targets.
    ExecutionTargetSorter(const Broker& b, const std::list<ComputingServiceType>& csList, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { addEntities(csList); }
    /// Constructor passing JobDescription and list of targets.
    ExecutionTargetSorter(const Broker& b, const JobDescription& j, const std::list<ComputingServiceType>& csList, const std::list<URL>& rejectEndpoints = std::list<URL>())
      : b(&b), rejectEndpoints(rejectEndpoints), current(targets.first.begin()) { set(j); addEntities(csList); }
    virtual ~ExecutionTargetSorter() {}

    /// Add an ExecutionTarget and rank it according to the Broker.
    void addEntity(const ExecutionTarget& et);
    /// Add an ComputingServiceType and rank it according to the Broker.
    void addEntity(const ComputingServiceType& cs);
    /// Add a list of ComputingServiceTypes and rank them according to the Broker.
    void addEntities(const std::list<ComputingServiceType>&);

    /// Reset to the first target in the ranked list.
    void reset() { current = targets.first.begin(); }
    /// Advance to the next target. Returns false if the current target is the last one.
    bool next() { if (!endOfList()) { ++current; }; return !endOfList(); }
    /// Returns true if current target is last in the list.
    bool endOfList() const { return current == targets.first.end(); }

    /// Returns current target.
    const ExecutionTarget& operator*() const { return *current; }
    /// Returns current target.
    const ExecutionTarget& getCurrentTarget() const { return *current; }
    /// Returns pointer to current target.
    const ExecutionTarget* operator->() const { return &*current; }

    /// Get sorted list of matching targets.
    const std::list<ExecutionTarget>& getMatchingTargets() const { return targets.first; }
    /// Get list of non-matching targets.
    const std::list<ExecutionTarget>& getNonMatchingTargets() const { return targets.second; }

    /// Clear lists of targets.
    void clear() { targets.first.clear(); targets.second.clear(); }
    /// Register that job was submitted to current target.
    /**
     * When brokering many jobs at once this method can be called after each
     * job submission to update the information held about the target it was
     * submitted to, such as number of free slots or free disk space.
     */
    void registerJobSubmission();

    /// Set a new Broker and recreate the ranked list of targets,
    void set(const Broker& newBroker) { b = &newBroker; sort(); }
    /// Set a new job description and recreate the ranked list of targets,
    void set(const JobDescription& j) { b->set(j); sort(); }
    /// Set a list of endpoints to reject when matching.
    void setRejectEndpoints(const std::list<URL>& newRejectEndpoints) { rejectEndpoints = newRejectEndpoints; }
    
  private:
    void sort();
    void insert(const ExecutionTarget& et);
    bool reject(const ExecutionTarget& et);
    
    const Broker* b;
    std::list<URL> rejectEndpoints;
    // Map of ExecutionTargets. first: matching; second: unsuitable.
    std::pair< std::list<ExecutionTarget>, std::list<ExecutionTarget> > targets;
    std::list<ExecutionTarget>::iterator current;
    
    static Logger logger;
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__
