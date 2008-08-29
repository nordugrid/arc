%{
#include <arc/loader/LoadableClass.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/Source.h>
%}

namespace ArcSec {
   
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
    
    class Evaluator {
        Evaluator (Arc::XMLNode*);
        Evaluator (const char *);
        void addPolicy(const Source& policy,const std::string& id = "");
        void addPolicy(Policy* policy,const std::string& id = "");
        Response* evaluate(Request* request);
        Response* evaluate(const Source& request);
        Response* evaluate(Request* request, const Source& policy);
        Response* evaluate(const Source& request, const Source& policy);
        Response* evaluate(Request* request, Policy* policyobj);
        Response* evaluate(const Source& request, Policy* policyobj);
    };

    class EvaluatorLoader {
    public:
        EvaluatorLoader();
        Evaluator* getEvaluator(const std::string& classname);
        Request* getRequest(const std::string& classname, const Source& requestsource);
        Policy* getPolicy(const std::string& classname, const Source& policysource);
    };

    class Policy  {
    public:
      Policy();
      Policy(Arc::XMLNode*);  
    };
    
    class Request {
    public:
      Request ();
      Request (const Source&);
    };
    

} //namespace ArcSec


