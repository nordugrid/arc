#ifdef SWIGPYTHON
%module security

%include "Arc.i"
#endif


/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%{
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/Source.h>
%}

%ignore ArcSec::ResponseList::operator[](int);

namespace ArcSec {

    %nodefaultctor Policy;
    class Policy  {};

    %nodefaultctor Request;
    class Request {};

    typedef enum {
      DECISION_PERMIT = 0,
      DECISION_DENY = 1,
      DECISION_INDETERMINATE = 2,
      DECISION_NOT_APPLICABLE = 3
    } Result;

    typedef struct {
      Result res;
    } ResponseItem;

    class ResponseList {
    public:
      int size() ;
      ResponseItem* getItem(int n);
      ResponseItem* operator[](int n);
      bool empty();
    };

    class Response {
    public:
      ResponseList& getResponseItems ();
    };

    class Source {
     public:
      Source(const Source& s):node(s.node);
      Source(Arc::XMLNode& xml);
      Source(std::istream& stream);
      Source(Arc::URL& url);
      Source(const std::string& str);
    };

    class SourceFile: public Source {
     public:
      SourceFile(const SourceFile& s):Source(s),stream(NULL);
      SourceFile(const char* name);
      SourceFile(const std::string& name);
    };

    class SourceURL: public Source {
     public:
      SourceURL(const SourceURL& s):Source(s),url(NULL);
      SourceURL(const char* url);
      SourceURL(const std::string& url);
    };

    %nodefaultctor Evaluator;
    %newobject Evaluator::evaluate;
    class Evaluator {
    public:
        void addPolicy(const Source& policy,const std::string& id = "");
        %apply SWIGTYPE *DISOWN {Policy *policy};
        void addPolicy(Policy* policy,const std::string& id = "");
        %clear Policy *policy;
        Response* evaluate(Request* request);
        Response* evaluate(const Source& request);
        Response* evaluate(const Source& request, const Source& policy);
        Response* evaluate(const Source& request, Policy* policyobj);
        Response* evaluate(Request* request, Policy* policyobj);
        Response* evaluate(Request* request, const Source& policy);
    };

    %newobject EvaluatorLoader::getEvaluator;
    %newobject EvaluatorLoader::getRequest;
    %newobject EvaluatorLoader::getPolicy;
    class EvaluatorLoader {
    public:
        EvaluatorLoader();
        Evaluator* getEvaluator(const std::string& classname);
        Request* getRequest(const std::string& classname, const Source& requestsource);
        Policy* getPolicy(const std::string& classname, const Source& policysource);
    };


} //namespace ArcSec


