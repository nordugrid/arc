// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RSLPARSER_H__
#define __ARC_RSLPARSER_H__

#include <list>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>

#include <arc/compute/JobDescriptionParserPlugin.h>

namespace Arc {

  class Logger;

  template<class T>
  class SourceLocation {
  public:
    SourceLocation(const T& v)
      : v(v), location(std::make_pair(-1, -1)) {};
    SourceLocation(const std::pair<int, int>& location, const T& v)
      : v(v), location(location) {};
    ~SourceLocation() {};
    bool operator==(const T& a) const { return v == a; }
    bool operator!=(const T& a) const { return v != a; }
    SourceLocation<T>& operator+=(const T& a) { v += a; return *this; }
    T v;
    std::pair<int, int> location; // (Line number, column number).
  };

  enum RSLBoolOp {
    RSLBoolError,
    RSLMulti,
    RSLAnd,
    RSLOr
  };

  enum RSLRelOp {
    RSLRelError,
    RSLEqual,
    RSLNotEqual,
    RSLLess,
    RSLGreater,
    RSLLessOrEqual,
    RSLGreaterOrEqual
  };

  class RSLValue {
  public:
    RSLValue(const std::pair<int, int>& location = std::make_pair(-1, -1)) : location(location) {};
    virtual ~RSLValue() {}
    const std::pair<int, int>& Location() const { return location; }
    RSLValue* Evaluate(std::map<std::string, std::string>& vars, JobDescriptionParserPluginResult& parsing_result) const;
    virtual void Print(std::ostream& os = std::cout) const = 0;
  protected:
    std::pair<int, int> location;
  };

  class RSLLiteral
    : public RSLValue {
  public:
    RSLLiteral(const SourceLocation<std::string>& str) : RSLValue(str.location), str(str.v) {};
    RSLLiteral(const std::string& str, const std::pair<int, int>& location = std::make_pair(-1, -1)) : RSLValue(location), str(str) {};
    RSLLiteral(const RSLLiteral& v) : RSLValue(v.location), str(v.str) {};
    ~RSLLiteral() {};
    void Print(std::ostream& os = std::cout) const;
    const std::string& Value() const { return str; };
  private:
    std::string str;
  };

  class RSLVariable
    : public RSLValue {
  public:
    RSLVariable(const SourceLocation<std::string>& var) : RSLValue(var.location), var(var.v) {};
    RSLVariable(const std::string& var, const std::pair<int, int>& location = std::make_pair(-1, -1)) : RSLValue(location), var(var) {};
    ~RSLVariable() {};
    void Print(std::ostream& os = std::cout) const;
    const std::string& Var() const { return var; };
  private:
    std::string var;
  };

  class RSLConcat
    : public RSLValue {
  public:
    RSLConcat(RSLValue *left, RSLValue *right, const std::pair<int, int>& location = std::make_pair(-1, -1)) : RSLValue(location), left(left), right(right) {};
    ~RSLConcat();
    void Print(std::ostream& os = std::cout) const;
    const RSLValue* Left() const {
      return left;
    }
    const RSLValue* Right() const {
      return right;
    }
  private:
    RSLValue *left;
    RSLValue *right;
  };

  class RSLList
    : public RSLValue {
  public:
    RSLList(const std::pair<int, int>& location = std::make_pair(-1, -1)) : RSLValue(location) {};
    ~RSLList();
    void Add(RSLValue *value);
    void Print(std::ostream& os = std::cout) const;
    std::list<RSLValue*>::iterator begin() {
      return values.begin();
    }
    std::list<RSLValue*>::iterator end() {
      return values.end();
    }
    std::list<RSLValue*>::const_iterator begin() const {
      return values.begin();
    }
    std::list<RSLValue*>::const_iterator end() const {
      return values.end();
    }
    std::list<RSLValue*>::size_type size() const {
      return values.size();
    }
  private:
    std::list<RSLValue*> values;
  };

  class RSLSequence
    : public RSLValue {
  public:
    RSLSequence(RSLList *seq, const std::pair<int, int>& location = std::make_pair(-1, -1)) : RSLValue(location), seq(seq) {};
    ~RSLSequence() { delete seq; };
    void Print(std::ostream& os = std::cout) const;
    std::list<RSLValue*>::iterator begin() {
      return seq->begin();
    }
    std::list<RSLValue*>::iterator end() {
      return seq->end();
    }
    std::list<RSLValue*>::const_iterator begin() const {
      return seq->begin();
    }
    std::list<RSLValue*>::const_iterator end() const {
      return seq->end();
    }
    std::list<RSLValue*>::size_type size() const {
      return seq->size();
    }
  private:
    RSLList *seq;
  };

  class RSL {
  public:
    RSL() {};
    virtual ~RSL() {};
    RSL* Evaluate(JobDescriptionParserPluginResult& parsing_result) const;
    virtual void Print(std::ostream& os = std::cout) const = 0;
  private:
    RSL* Evaluate(std::map<std::string, std::string>& vars, JobDescriptionParserPluginResult& parsing_result) const;
  };

  class RSLBoolean
    : public RSL {
  public:
    RSLBoolean(SourceLocation<RSLBoolOp> op) : RSL(), op(op) {};
    ~RSLBoolean();
    void Add(RSL *condition);
    void Print(std::ostream& os = std::cout) const;
    RSLBoolOp Op() const { return op.v; }
    const std::pair<int, int>& OpLocation() const { return op.location; }
    std::list<RSL*>::iterator begin() { return conditions.begin(); }
    std::list<RSL*>::iterator end() { return conditions.end(); }
    std::list<RSL*>::const_iterator begin() const { return conditions.begin(); }
    std::list<RSL*>::const_iterator end() const { return conditions.end(); }
    std::list<RSL*>::size_type size() const { return conditions.size(); }
  private:
    SourceLocation<RSLBoolOp> op;
    std::list<RSL*> conditions;
  };

  class RSLCondition
    : public RSL {
  public:
    RSLCondition(const SourceLocation<std::string>& attr, const SourceLocation<RSLRelOp>& op, RSLList *values)
      : RSL(), attr(attr), op(op), values(values) { init(); };
    RSLCondition(const std::string& attr, RSLRelOp op, RSLList *values)
      : RSL(), attr(SourceLocation<std::string>(attr)), op(SourceLocation<RSLRelOp>(op)), values(values) { init(); };
    ~RSLCondition() { delete values; }
    void Print(std::ostream& os = std::cout) const;
    const std::string& Attr() const { return attr.v; }
    const std::pair<int, int>& AttrLocation() const { return attr.location; }
    RSLRelOp Op() const { return op.v; }
    std::pair<int, int> OpLocation() const { return op.location; }
    const RSLList& List() const { return *values; }
    std::list<RSLValue*>::iterator begin() { return values->begin(); }
    std::list<RSLValue*>::iterator end() { return values->end(); }
    std::list<RSLValue*>::const_iterator begin() const { return values->begin(); }
    std::list<RSLValue*>::const_iterator end() const { return values->end(); }
    std::list<RSLValue*>::size_type size() const { return values->size(); }
  private:
    void init();
    SourceLocation<std::string> attr;
    SourceLocation<RSLRelOp> op;
    RSLList *values;
  };

  class RSLParser {
  public:
    RSLParser(const std::string& s)
      : s(s), n(0), parsed(NULL), evaluated(NULL),
        parsing_result(JobDescriptionParserPluginResult::WrongLanguage) {};
    ~RSLParser();
    // The Parse method returns a pointer to an RSL object containing a
    // parsed representation of the rsl string given to the constructor.
    // The pointer is owned by the RSLParser object and should not be
    // deleted by the caller. The pointer is valid until the RSLParser
    // object goes out of scope.
    // If the evaluate flag is true the returned RSL object has been
    // evaluated to resolve RSL substitutions and evaluate concatenation
    // operations.
    // The parsing and evaluation is done on the first call to the Parse
    // method. Subsequent calls will simply return stored results.
    // If the rsl string can not be parsed or evaluated a NULL pointer
    // is returned.
    // It is possible that an rsl string can be parsed but not evaluated.
    const RSL* Parse(bool evaluate = true);
    const JobDescriptionParserPluginResult& GetParsingResult() const { return parsing_result; };
  private:
    void SkipWSAndComments();
    SourceLocation<RSLBoolOp> ParseBoolOp();
    SourceLocation<RSLRelOp> ParseRelOp();
    SourceLocation<std::string> ParseString(int& status);
    RSLList* ParseList();
    RSL* ParseRSL();
    std::pair<int, int> GetLinePosition(std::string::size_type pos) const;
    template<class T>
    SourceLocation<T> toSourceLocation(const T& v, std::string::size_type offset = 1) const;
    const std::string s;
    std::string::size_type n;
    RSL *parsed;
    RSL *evaluated;

    JobDescriptionParserPluginResult parsing_result;
    std::map<std::string::size_type, std::string::size_type> comments_positions;
  };

  std::ostream& operator<<(std::ostream& os, const RSLBoolOp op);
  std::ostream& operator<<(std::ostream& os, const RSLRelOp op);
  std::ostream& operator<<(std::ostream& os, const RSLValue& value);
  std::ostream& operator<<(std::ostream& os, const RSL& rsl);

} // namespace Arc

#endif // __ARC_RSLPARSER_H__
