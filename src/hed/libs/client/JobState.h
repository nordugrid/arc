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

  // Each plugin should contain a class which inherits from this class.
  // Only the constructor and the static method StateMap is supposed to overridden.

  class JobState {
    public:
      // Order is important!
#define JOBSTATE_TABLE \
        JOBSTATE_X(UNDEFINED, "Undefined")\
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
        JOBSTATE_X(DELETED, "Deleted")

#define JOBSTATE_X(a, b) a,
      enum StateType { JOBSTATE_TABLE };
#undef JOBSTATE_X
      static const std::string StateTypeString[];


      JobState() : type(UNDEFINED) {}
      JobState(const std::string& state, JobState::StateType (*map)(const std::string&))
        : state(state), type((*map)(state)) {};

      JobState& operator=(const JobState& newState) { type = newState.type; state = newState.state; return *this; }

      bool operator==(const StateType& st) const { return type == st; }
      bool operator!=(const StateType& st) const { return type != st; }
      bool operator<(const StateType& st) const { return type < st; }
      bool operator>(const StateType& st) const { return type > st; }
      bool operator<=(const StateType& st) const { return type <= st; }
      bool operator>=(const StateType& st) const { return type >= st; }

      std::string operator()() const { return state; } // State reported by Plugin.

      std::string GetGeneralState() const { return StateTypeString[type]; } // General state.

    private:
      StateType type;
      std::string state;
  };

  typedef JobState::StateType (*JobStateMap)(const std::string&);
}

#endif // __ARC_JOBSTATE_H__
