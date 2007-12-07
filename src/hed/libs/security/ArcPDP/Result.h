namespace ArcSec {

  ///Evaluation result  
  enum Result {
    /**Permit*/
    DECISION_PERMIT = 0,
    /**Deny*/
    DECISION_DENY = 1,
    /**Indeterminate, because of the Indeterminate from the "Matching"*/
    DECISION_INDETERMINATE = 2,
    /**Not_Applicable, means the the request tuple <Subject, Resource, Action, Context> does not match the rule. So there is no way to get to the "Permit"/"Deny" effect. */
    DECISION_NOT_APPLICABLE = 3
  };

  ///Match result
  enum MatchResult {
    /**Match, the request tuple <Subject, Resource, Action, Context> matches the rule*/
    MATCH = 0,
    /**No_Match, the request tuple <Subject, Resource, Action, Context> does not match the rule*/
    NO_MATCH = 1,
    /**Indeterminate, means that the request tuple <Subject, Resource, Action, Context> matches the 
rule, but in terms of the other "Condition", the tuple does not match. So far, the Indeterminate has
 no meaning in the existing code (will never be switched to)*/
    INDETERMINATE = 2
  };

  ///Struct to record the xml node and effect, which will be used by Evaluator to get the information about which rule/policy(in xmlnode) is satisfied
  typedef struct {
    Arc::XMLNode node;
    std::string effect;
  } EvalResult; 

}

