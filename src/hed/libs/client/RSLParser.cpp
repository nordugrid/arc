#include <exception>
#include <sstream>

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "RSLParser.h"

namespace Arc {

  RSLValue::RSLValue() {}

  RSLValue::~RSLValue() {}

  RSLValue* RSLValue::Evaluate(std::map<std::string, std::string>& vars) const {
    const RSLLiteral *n;
    const RSLVariable *v;
    const RSLConcat *c;
    const RSLList *l;
    const RSLSequence *s;
    if ((n = dynamic_cast<const RSLLiteral*>(this)))
      return new RSLLiteral(n->Value());
    else if ((v = dynamic_cast<const RSLVariable*>(this))) {
      std::map<std::string, std::string>::iterator it = vars.find(v->Var());
      return new RSLLiteral((it != vars.end()) ? it->second : "");
    }
    else if ((c = dynamic_cast<const RSLConcat*>(this))) {
      RSLValue *left = c->Left()->Evaluate(vars);
      if (!left) {
	std::stringstream ss;
	ss << *c->Left();
	logger.msg(ERROR, "Can't evaluate left operand for RSL "
		   "concatenation: %s", ss.str());
	return NULL;
      }
      RSLValue *right = c->Right()->Evaluate(vars);
      if (!right) {
	std::stringstream ss;
	ss << *c->Right();
	logger.msg(ERROR, "Can't evaluate right operand for RSL "
		   "concatenation: %s", ss.str());
	delete left;
	return NULL;
      }
      RSLLiteral *nleft = dynamic_cast<RSLLiteral*>(left);
      if (!nleft) {
	std::stringstream ss;
	ss << *left;
	logger.msg(ERROR, "Left operand for RSL concatenation does not "
		   "evaluate to a literal: %s", ss.str());
	delete left;
	delete right;
	return NULL;
      }
      RSLLiteral *nright = dynamic_cast<RSLLiteral*>(right);
      if (!nright) {
	std::stringstream ss;
	ss << *right;
	logger.msg(ERROR, "Right operand for RSL concatenation does not "
		   "evaluate to a literal: %s", ss.str());
	delete left;
	delete right;
	return NULL;
      }
      RSLLiteral *result = new RSLLiteral(nleft->Value() + nright->Value());
      delete left;
      delete right;
      return result;
    }
    else if ((l = dynamic_cast<const RSLList*>(this))) {
      RSLList *result = new RSLList;
      for (std::list<RSLValue*>::const_iterator it = l->begin();
	   it != l->end(); it++) {
	RSLValue *value = (*it)->Evaluate(vars);
	if (!value) {
	  std::stringstream ss;
	  ss << **it;
	  logger.msg(ERROR, "Can't evaluate RSL list member: %s", ss.str());
	  delete result;
	  return NULL;
	}
	result->Add(value);
      }
      return result;
    }
    else if ((s = dynamic_cast<const RSLSequence*>(this))) {
      RSLList *result = new RSLList;
      for (std::list<RSLValue*>::const_iterator it = s->begin();
	   it != s->end(); it++) {
	RSLValue *value = (*it)->Evaluate(vars);
	if (!value) {
	  std::stringstream ss;
	  ss << **it;
	  logger.msg(ERROR, "Can't evaluate RSL sequence member: %s", ss.str());
	  delete result;
	  return NULL;
	}
	result->Add(value);
      }
      return new RSLSequence(result);
    }
    else {
      logger.msg(ERROR, "Unknown RSL value type - should not happen");
      return NULL;
    }
  }

  RSLLiteral::RSLLiteral(const std::string& str)
    : RSLValue(),
      str(str) {}

  RSLLiteral::~RSLLiteral() {}

  void RSLLiteral::Print(std::ostream& os) const {
    std::string s(str);
    std::string::size_type pos = 0;
    while ((pos = s.find('"', pos)) != std::string::npos) {
      s.insert(pos, 1, '"');
      pos += 2;
    }
    os << '"' << s << '"';
  }

  RSLVariable::RSLVariable(const std::string& var)
    : RSLValue(),
      var(var) {}

  RSLVariable::~RSLVariable() {}

  void RSLVariable::Print(std::ostream& os) const {
    os << "$(" << var << ')';
  }

  RSLConcat::RSLConcat(RSLValue *left, RSLValue *right)
    : RSLValue(),
      left(left),
      right(right) {}

  RSLConcat::~RSLConcat() {
    delete left;
    delete right;
  }

  void RSLConcat::Print(std::ostream& os) const {
    os << *left << " # " << *right;
  }

  RSLList::RSLList()
    : RSLValue() {}

  RSLList::~RSLList() {
    for (std::list<RSLValue*>::iterator it = begin(); it != end(); it++)
      delete *it;
  }

  void RSLList::Add(RSLValue *value) {
    values.push_back(value);
  }

  void RSLList::Print(std::ostream& os) const {
    for (std::list<RSLValue*>::const_iterator it = begin();
	 it != end(); it++) {
      if (it != begin())
	os << " ";
      os << **it;
    }
  }

  RSLSequence::RSLSequence(RSLList *seq)
    : RSLValue(),
      seq(seq) {}

  RSLSequence::~RSLSequence() {
    delete seq;
  }

  void RSLSequence::Print(std::ostream& os) const {
    os << "( " << *seq << " )";
  }

  RSL::RSL() {}

  RSL::~RSL() {}

  RSL* RSL::Evaluate() const {
    const RSLBoolean *b = dynamic_cast<const RSLBoolean*>(this);
    if (b && (b->Op() == RSLMulti)) {
      RSLBoolean *result = new RSLBoolean(RSLMulti);
      for (std::list<RSL*>::const_iterator it = b->begin();
	   it != b->end(); it++) {
	RSL *rsl = (*it)->Evaluate();
	if (!rsl) {
	  std::stringstream ss;
	  ss << **it;
	  logger.msg(ERROR, "RLS (inside multi) could not be evaluated: %s",
		     ss.str());
	  delete rsl;
	  return NULL;
	}
	result->Add(rsl);
      }
      return result;
    }
    else {
      std::map<std::string, std::string> vars;
      RSL *result = Evaluate(vars);
      if (!result) {
	std::stringstream ss;
	ss << *this;
	logger.msg(ERROR, "RLS could not be evaluated: %s", ss.str());
	return NULL;
      }
      return result;
    }
  }

  RSL* RSL::Evaluate(std::map<std::string, std::string>& vars) const {
    const RSLBoolean *b;
    const RSLCondition *c;
    if ((b = dynamic_cast<const RSLBoolean*>(this))) {
      if (b->Op() == RSLMulti) {
	logger.msg(ERROR, "RSL multi operator not at top level");
	return NULL;
      }
      else {
	RSLBoolean *result = new RSLBoolean(b->Op());
	std::map<std::string, std::string> vars2(vars);
	for (std::list<RSL*>::const_iterator it = b->begin();
	     it != b->end(); it++) {
	  RSL *rsl = (*it)->Evaluate(vars2);
	  if (!rsl) {
	    std::stringstream ss;
	    ss << **it;
	    logger.msg(ERROR, "Can't evaluate RLS fragment: %s", ss.str());
	    delete rsl;
	    return NULL;
	  }
	  result->Add(rsl);
	}
	return result;
      }
    }
    else if ((c = dynamic_cast<const RSLCondition*>(this))) {
      RSLList *l = new RSLList;
      if (c->Attr() == "rslsubstitution")
	for (std::list<RSLValue*>::const_iterator it = c->begin();
	     it != c->end(); it++) {
	  const RSLSequence *s = dynamic_cast<const RSLSequence*>(*it);
	  if (!s) {
	    std::stringstream ss;
	    ss << **it;
	    logger.msg(ERROR, "RLS substitution is not a sequence: %s",
		       ss.str());
	    delete l;
	    return NULL;
	  }
	  if (s->size() != 2) {
	    std::stringstream ss;
	    ss << *s;
	    logger.msg(ERROR, "RLS substitution sequence is not of "
		       "length 2: %s", ss.str());
	    delete l;
	    return NULL;
	  }
	  std::list<RSLValue*>::const_iterator it2 = s->begin();
	  RSLValue *var = (*it2)->Evaluate(vars);
	  if (!var) {
	    std::stringstream ss;
	    ss << **it2;
	    logger.msg(ERROR, "Can't evaluate RLS substitution variable "
		       "name: %s", ss.str());
	    delete l;
	    return NULL;
	  }
	  it2++;
	  RSLValue *val = (*it2)->Evaluate(vars);
	  if (!val) {
	    std::stringstream ss;
	    ss << **it2;
	    logger.msg(ERROR, "Can't evaluate RLS substitution variable "
		       "value: %s", ss.str());
	    delete l;
	    delete var;
	    return NULL;
	  }
	  RSLLiteral *nvar = dynamic_cast<RSLLiteral*>(var);
	  if (!nvar) {
	    std::stringstream ss;
	    ss << *var;
	    logger.msg(ERROR, "RLS substitution variable name does not "
		       "evaluate to a literal: %s", ss.str());
	    delete l;
	    delete var;
	    delete val;
	    return NULL;
	  }
	  RSLLiteral *nval = dynamic_cast<RSLLiteral*>(val);
	  if (!nval) {
	    std::stringstream ss;
	    ss << *val;
	    logger.msg(ERROR, "RLS substitution variable value does not "
		       "evaluate to a literal: %s", ss.str());
	    delete l;
	    delete var;
	    delete val;
	    return NULL;
	  }
	  vars[nvar->Value()] = nval->Value();
	  RSLList *seq = new RSLList;
	  seq->Add(var);
	  seq->Add(val);
	  l->Add(new RSLSequence(seq));
	}
      else
	for (std::list<RSLValue*>::const_iterator it = c->begin();
	     it != c->end(); it++) {
	  RSLValue *v = (*it)->Evaluate(vars);
	  if (!v) {
	    std::stringstream ss;
	    ss << **it;
	    logger.msg(ERROR, "Can't evaluate RSL condition value: %s",
		       ss.str());
	    delete l;
	    return NULL;
	  }
	  l->Add(v);
	}
      return new RSLCondition(c->Attr(), c->Op(), l);
    }
    else {
      logger.msg(ERROR, "Unknown RSL type - should not happen");
      return NULL;
    }
  }

  RSLBoolean::RSLBoolean(RSLBoolOp op)
    : RSL(),
      op(op) {}

  RSLBoolean::~RSLBoolean() {
    for (std::list<RSL*>::iterator it = begin(); it != end(); it++)
      delete *it;
  }

  void RSLBoolean::Add(RSL *condition) {
    conditions.push_back(condition);
  }

  void RSLBoolean::Print(std::ostream& os) const {
    os << op;
    for (std::list<RSL*>::const_iterator it = begin(); it != end(); it++)
      os << "( " << **it << " )";
  }

  RSLCondition::RSLCondition(const std::string& attr,
			     RSLRelOp op, RSLList *values)
    : RSL(),
      attr(attr),
      op(op),
      values(values) {
    // Normalize the attribute name
    // Does the same thing as globus_rsl_assist_attributes_canonicalize,
    // i.e. lowercase the attribute name and remove underscores
    this->attr = lower(this->attr);
    std::string::size_type pos = 0;
    while ((pos = this->attr.find('_', pos)) != std::string::npos)
      this->attr.erase(pos, 1);
  }

  RSLCondition::~RSLCondition() {
    delete values;
  }

  void RSLCondition::Print(std::ostream& os) const {
    os << attr << ' ' << op << ' ' << *values;
  }

  RSLParser::RSLParser(const std::string s)
    : s(s),
      n(0),
      parsed(NULL),
      evaluated(NULL) {}

  RSLParser::~RSLParser() {
    if (parsed)
      delete parsed;
    if (evaluated)
      delete evaluated;
  }

  const RSL* RSLParser::Parse(bool evaluate) {
    if (n == 0) {
      std::string::size_type pos = 0;
      while ((pos = s.find("(*", pos)) != std::string::npos) {
	std::string::size_type pos2 = s.find("*)", pos);
	if (pos2 == std::string::npos) {
	  logger.msg(ERROR, "End of comment not found at position %ld", pos);
	  return NULL;
	}
	s.replace(pos, pos2 - pos + 2, 1, ' ');
      }
      parsed = ParseRSL();
      if (!parsed)
	logger.msg(DEBUG, "RSL parsing failed at position %ld", n);
      else {
	SkipWS();
	if (n != std::string::npos) {
	  logger.msg(ERROR, "Junk at end of RSL at position %ld", n);
	  delete parsed;
	}
      }
      if (parsed)
	evaluated = parsed->Evaluate();
    }
    return evaluate ? evaluated : parsed;
  }

  void RSLParser::SkipWS() {
    n = s.find_first_not_of(" \t\n\v\f\r", n);
  }

  RSLBoolOp RSLParser::ParseBoolOp() {
    switch (s[n]) {
    case '+':
      n++;
      return RSLMulti;
      break;

    case '&':
      n++;
      return RSLAnd;
      break;

    case '|':
      n++;
      return RSLOr;
      break;

    default:
      return RSLBoolError;
      break;
    }
  }

  RSLRelOp RSLParser::ParseRelOp() {
    switch (s[n]) {
    case '=':
      n++;
      return RSLEqual;
      break;

    case '!':
      if (s[n + 1] == '=') {
	n += 2;
	return RSLNotEqual;
      }
      return RSLRelError;
      break;

    case '<':
      n++;
      if (s[n] == '=') {
	n++;
	return RSLLessOrEqual;
      }
      return RSLLess;
      break;

    case '>':
      n++;
      if (s[n] == '=') {
	n++;
	return RSLGreaterOrEqual;
      }
      return RSLGreater;
      break;

    default:
      return RSLRelError;
      break;
    }
  }

  std::string RSLParser::ParseString(int& status) {
    // status: 1 - OK, 0 - not a string, -1 - error
    if (s[n] == '\'') {
      std::string str;
      do {
	std::string::size_type pos = s.find('\'', n + 1);
	if (pos == std::string::npos) {
	  logger.msg(ERROR, "End of single quoted string not found "
		     "at position %ld", n);
	  status = -1;
	  return "";
	}
	str += s.substr(n + 1, pos - n - 1);
	n = pos + 1;
	if (s[n] == '\'')
	  str += '\'';
      } while (s[n] == '\'');
      status = 1;
      return str;
    }
    else if (s[n] == '"') {
      std::string str;
      do {
	std::string::size_type pos = s.find('"', n + 1);
	if (pos == std::string::npos) {
	  logger.msg(ERROR, "End of double quoted string not found "
		     "at position %ld", n);
	  status = -1;
	  return "";
	}
	str += s.substr(n + 1, pos - n - 1);
	n = pos + 1;
	if (s[n] == '"')
	  str += '"';
      } while (s[n] == '"');
      status = 1;
      return str;
    }
    else if (s[n] == '^') {
      n++;
      char delim = s[n];
      std::string str;
      do {
	std::string::size_type pos = s.find(delim, n + 1);
	if (pos == std::string::npos) {
	  logger.msg(ERROR, "End of user delimiter quoted string not found "
		     "at position %ld", n);
	  status = -1;
	  return "";
	}
	str += s.substr(n + 1, pos - n - 1);
	n = pos + 1;
	if (s[n] == delim)
	  str += delim;
      } while (s[n] == delim);
      status = 1;
      return str;
    }
    else {
      std::string::size_type pos =
	s.find_first_of("+&|()=<>!\"'^#$ \t\n\v\f\r", n);
      if (pos == n) {
	status = 0;
	return "";
      }
      std::string str = s.substr(n, pos - n);
      n = pos;
      status = 1;
      return str;
    }
  }

  RSLList* RSLParser::ParseList() {

    RSLList *values = new RSLList();
    RSLValue *left = NULL;
    RSLValue *right = NULL;

    try {
      int concat = 0; // 0 = No, 1 = Explicit, 2 = Implicit
      do {
	right = NULL;
	int nextconcat = 0;
	std::string::size_type nsave = n;
	SkipWS();
	if (n != nsave)
	  concat = 0;
	if (s[n] == '#') {
	  n++;
	  SkipWS();
	  concat = 1;
	}
	if (s[n] == '(') {
	  n++;
	  RSLList *seq = ParseList();
	  SkipWS();
	  if (s[n] != ')') {
	    logger.msg(ERROR, "Expected ) at position %ld", n);
	    throw std::exception();
	  }
	  n++;
	  right = new RSLSequence(seq);
	}
	else if (s[n] == '$') {
	  n++;
	  SkipWS();
	  if (s[n] != '(') {
	    logger.msg(ERROR, "Expected ( at position %ld", n);
	    throw std::exception();
	  }
	  n++;
	  SkipWS();
	  int status;
	  std::string var = ParseString(status);
	  if (status != 1) {
	    logger.msg(ERROR, "Expected variable name at position %ld", n);
	    throw std::exception();
	  }
	  if (var.find_first_of("+&|()=<>!\"'^#$") != std::string::npos) {
	    logger.msg(ERROR, "Variable name contains invalid character "
		       "at position %ld", n);
	    throw std::exception();
	  }
	  SkipWS();
	  if (s[n] != ')') {
	    logger.msg(ERROR, "Expected ) at position %ld", n);
	    throw std::exception();
	  }
	  n++;
	  right = new RSLVariable(var);
	  nextconcat = 2;
	}
	else {
	  int status;
	  std::string val = ParseString(status);
	  if (status == -1) {
	    logger.msg(ERROR, "Broken string at position %ld", n);
	    throw std::exception();
	  }
	  right = (status == 1) ? new RSLLiteral(val) : NULL;
	  nextconcat = right ? 2 : 0;
	}
	if (concat == 0) {
	  if (left)
	    values->Add(left);
	  left = right;
	}
	else if (concat == 1) {
	  if (!left) {
	    logger.msg(ERROR, "no left operand for concatenation operator "
		       "at position %ld", n);
	    throw std::exception();
	  }
	  if (!right) {
	    logger.msg(ERROR, "no right operand for concatenation operator "
		       "at position %ld", n);
	    throw std::exception();
	  }
	  left = new RSLConcat(left, right);
	}
	else if (concat == 2) {
	  if (left) {
	    if (right)
	      left = new RSLConcat(left, right);
	  }
	  else
	    left = right;
	}
	concat = nextconcat;
      } while (left || right);
      return values;
    }
    catch (std::exception e) {
      if (values)
	delete values;
      if (left)
	delete left;
      if (right)
	delete right;
      return NULL;
    }
  }

  RSL* RSLParser::ParseRSL() {
    SkipWS();
    RSLBoolOp op = ParseBoolOp();
    if (op != RSLBoolError) {
      SkipWS();
      RSLBoolean *b = new RSLBoolean(op);
      do {
	if (s[n] != '(') {
	  logger.msg(ERROR, "Expected ( at position %ld", n);
	  delete b;
	  return NULL;
	}
	n++;
	SkipWS();
	RSL *rsl = ParseRSL();
	if (!rsl) {
	  logger.msg(ERROR, "RSL parsing error at position %ld", n);
	  delete b;
	  return NULL;
	}
	b->Add(rsl);
	SkipWS();
	if (s[n] != ')') {
	  logger.msg(ERROR, "Expected ) at position %ld", n);
	  delete b;
	  return NULL;
	}
	n++;
	SkipWS();
      } while (s[n] == '(');
      return b;
    }
    else {
      int status;
      std::string attr = ParseString(status);
      if (status != 1) {
	logger.msg(ERROR, "Expected attribute name at position %ld", n);
	return NULL;
      }
      if (attr.find_first_of("+&|()=<>!\"'^#$") != std::string::npos) {
	logger.msg(ERROR, "Attribute name contains invalid character "
		   "at position %ld", n);
	return NULL;
      }
      SkipWS();
      RSLRelOp op = ParseRelOp();
      if (op == RSLRelError) {
	logger.msg(ERROR, "Expected relation operator at position %ld", n);
	return NULL;
      }
      SkipWS();
      RSLList *values = ParseList();
      if (!values) {
	logger.msg(ERROR, "RSL parsing error at position %ld", n);
	return NULL;
      }
      RSLCondition *c = new RSLCondition(attr, op, values);
      return c;
    }
  }

  Logger RSLValue::logger(Logger::getRootLogger(), "RSLValue");
  Logger RSL::logger(Logger::getRootLogger(), "RSL");
  Logger RSLParser::logger(Logger::getRootLogger(), "RSLParser");

  std::ostream& operator<<(std::ostream& os, const RSLBoolOp op) {
    switch (op) {
    case RSLBoolError:
      return os << "This should not happen";

    case RSLMulti:
      return os << '+';

    case RSLAnd:
      return os << '&';

    case RSLOr:
      return os << '|';
    }
  }

  std::ostream& operator<<(std::ostream& os, const RSLRelOp op) {
    switch (op) {
    case RSLRelError:
      return os << "This should not happen";

    case RSLEqual:
      return os << '=';

    case RSLNotEqual:
      return os << "!=";

    case RSLLess:
      return os << '<';

    case RSLGreater:
      return os << '>';

    case RSLLessOrEqual:
      return os << "<=";

    case RSLGreaterOrEqual:
      return os << ">=";
    }
  }

  std::ostream& operator<<(std::ostream& os, const RSLValue& value) {
    value.Print(os);
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RSL& rsl) {
    rsl.Print(os);
    return os;
  }

} // namespace Arc
