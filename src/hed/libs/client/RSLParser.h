// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_RSLPARSER_H__
#define __ARC_RSLPARSER_H__

#include <list>
#include <map>
#include <string>
#include <iostream>

namespace Arc {

  class Logger;

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
    RSLValue();
    virtual ~RSLValue();
    RSLValue* Evaluate(std::map<std::string, std::string>& vars) const;
    virtual void Print(std::ostream& os = std::cout) const = 0;
  private:
    static Logger logger;
  };

  class RSLLiteral
    : public RSLValue {
  public:
    RSLLiteral(const std::string& str);
    ~RSLLiteral();
    void Print(std::ostream& os = std::cout) const;
    const std::string& Value() const {
      return str;
    }
  private:
    std::string str;
  };

  class RSLVariable
    : public RSLValue {
  public:
    RSLVariable(const std::string& var);
    ~RSLVariable();
    void Print(std::ostream& os = std::cout) const;
    const std::string& Var() const {
      return var;
    }
  private:
    std::string var;
  };

  class RSLConcat
    : public RSLValue {
  public:
    RSLConcat(RSLValue *left, RSLValue *right);
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
    RSLList();
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
    RSLSequence(RSLList *seq);
    ~RSLSequence();
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
    RSL();
    virtual ~RSL();
    RSL* Evaluate() const;
    virtual void Print(std::ostream& os = std::cout) const = 0;
  private:
    RSL* Evaluate(std::map<std::string, std::string>& vars) const;
    static Logger logger;
  };

  class RSLBoolean
    : public RSL {
  public:
    RSLBoolean(RSLBoolOp op);
    ~RSLBoolean();
    void Add(RSL *condition);
    void Print(std::ostream& os = std::cout) const;
    RSLBoolOp Op() const {
      return op;
    }
    std::list<RSL*>::iterator begin() {
      return conditions.begin();
    }
    std::list<RSL*>::iterator end() {
      return conditions.end();
    }
    std::list<RSL*>::const_iterator begin() const {
      return conditions.begin();
    }
    std::list<RSL*>::const_iterator end() const {
      return conditions.end();
    }
    std::list<RSL*>::size_type size() const {
      return conditions.size();
    }
  private:
    RSLBoolOp op;
    std::list<RSL*> conditions;
  };

  class RSLCondition
    : public RSL {
  public:
    RSLCondition(const std::string& attr, RSLRelOp op, RSLList *values);
    ~RSLCondition();
    void Print(std::ostream& os = std::cout) const;
    const std::string& Attr() const {
      return attr;
    }
    const RSLRelOp Op() const {
      return op;
    }
    std::list<RSLValue*>::iterator begin() {
      return values->begin();
    }
    std::list<RSLValue*>::iterator end() {
      return values->end();
    }
    std::list<RSLValue*>::const_iterator begin() const {
      return values->begin();
    }
    std::list<RSLValue*>::const_iterator end() const {
      return values->end();
    }
    std::list<RSLValue*>::size_type size() const {
      return values->size();
    }
  private:
    std::string attr;
    RSLRelOp op;
    RSLList *values;
  };

  class RSLParser {
  public:
    RSLParser(const std::string s);
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
  private:
    void SkipWS();
    RSLBoolOp ParseBoolOp();
    RSLRelOp ParseRelOp();
    std::string ParseString(int& status);
    RSLList* ParseList();
    RSL* ParseRSL();
    std::string s;
    std::string::size_type n;
    RSL *parsed;
    RSL *evaluated;
    static Logger logger;
  };

  std::ostream& operator<<(std::ostream& os, const RSLBoolOp op);
  std::ostream& operator<<(std::ostream& os, const RSLRelOp op);
  std::ostream& operator<<(std::ostream& os, const RSLValue& value);
  std::ostream& operator<<(std::ostream& os, const RSL& rsl);

} // namespace Arc

#endif // __ARC_RSLPARSER_H__
