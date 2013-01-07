#ifndef __ARC_JOBSTATE_H__
#define __ARC_JOBSTATE_H__

#include <string>
#include <map>

#ifdef JOBSTATE_TABLE
#undef JOBSTATE_TABLE
#endif

#ifdef JOBSTATE_X
#undef JOBSTATE_X
#endif

namespace Arc {

  /** ARC general state model.
   * The class comprise the general state model of the ARC-lib, and are herein
   * used to compare job states from the different middlewares supported by the
   * plugin structure of the ARC-lib. Which is why every ACC plugin should
   * contain a class derived from this class. The derived class should consist
   * of a constructor and a mapping function (a JobStateMap) which maps a
   * std::string to a JobState:StateType. An example of a constructor in a
   * plugin could be:
   * JobStatePlugin::JobStatePluging(const std::string& state) : JobState(state, &pluginStateMap) {}
   * where &pluginStateMap is a reference to the JobStateMap defined by the
   * derived class.
   * 
   * \ingroup compute
   * \headerfile JobState.h arc/compute/JobState.h 
   */
  class JobState {
  public:
#define JOBSTATE_TABLE \
    JOBSTATE_X(ACCEPTED, "Accepted")\
    JOBSTATE_X(PREPARING, "Preparing")\
    JOBSTATE_X(SUBMITTING, "Submitting")\
    JOBSTATE_X(HOLD, "Hold")\
    JOBSTATE_X(QUEUING, "Queuing")\
    JOBSTATE_X(RUNNING, "Running")\
    JOBSTATE_X(FINISHING, "Finishing")\
    JOBSTATE_X(FINISHED, "Finished")\
    JOBSTATE_X(KILLED, "Killed")\
    JOBSTATE_X(FAILED, "Failed")\
    JOBSTATE_X(DELETED, "Deleted")\
    JOBSTATE_X(OTHER, "Other")

#define JOBSTATE_X(a, b) , a
    enum StateType { UNDEFINED JOBSTATE_TABLE };
#undef JOBSTATE_X
    static const std::string StateTypeString[];

    JobState() : ssf(FormatSpecificState), type(UNDEFINED) {}
    
    JobState& operator=(const JobState& js) { type = js.type; state = js.state; ssf = js.ssf; return *this; }

    operator bool() const { return type != UNDEFINED; }
    operator StateType() const { return type; }
    bool operator!() const { return type == UNDEFINED; }
    bool operator==(const StateType& st) const { return type == st; }
    bool operator!=(const StateType& st) const { return type != st; }

    /// Check if state is finished
    /**
     * @return true is returned if the StateType is equal to FINISHED, KILLED,
     *  FAILED or DELETED, otherwise false is returned.
     **/
    bool IsFinished() const { return type == FINISHED || type == KILLED || type == FAILED || type == DELETED; }

    /// Unformatted specific job state
    /**
     * Get the unformatted specific job state as returned by the CE.
     * 
     * @return job state as returned by CE
     * @see GetSpecificState
     * @see GetGeneralState
     **/
    const std::string& operator()() const { return state; }

    /// General string representation of job state
    /**
     * Get the string representation of the job state as mapped to the
     * libarccompute job state model.
     * 
     * @return string representing general job state
     * @see enum StateType
     * @see GetSpecificState
     **/
    const std::string& GetGeneralState() const { return StateTypeString[type]; }
    
    /// Specific string representation of job state
    /**
     * Get the string representation of the job state as returned by
     * the CE service possibly formatted to a human readable string.
     * 
     * @return string representing specific, possibly formatted, job state
     * @see GetGeneralState
     * @see operator()
     **/
    std::string GetSpecificState() const { return ssf(state); }

    static StateType GetStateType(const std::string& state);

    friend class Job;

  protected:
    typedef std::string (*SpecificStateFormater)(const std::string&);
    SpecificStateFormater ssf;
    static std::string FormatSpecificState(const std::string& state) { return state; }
    
    JobState(const std::string& state, JobState::StateType (*map)(const std::string&), SpecificStateFormater ssf = FormatSpecificState)
      : ssf(ssf), state(state), type((*map)(state)) {};
    std::string state;
    StateType type;
  };

  /**
   * \ingroup compute
   * \headerfile JobState.h arc/compute/JobState.h 
   */
  typedef JobState::StateType (*JobStateMap)(const std::string&);
}

#endif // __ARC_JOBSTATE_H__
