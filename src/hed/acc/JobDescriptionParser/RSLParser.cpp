// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <exception>
#include <sstream>

#include <arc/StringConv.h>

#include "RSLParser.h"

namespace Arc {

  RSLValue* RSLValue::Evaluate(std::map<std::string, std::string>& vars, JobDescriptionParserPluginResult& parsing_result) const {
    const RSLLiteral *n;
    const RSLVariable *v;
    const RSLConcat *c;
    const RSLList *l;
    const RSLSequence *s;
    if ((n = dynamic_cast<const RSLLiteral*>(this)))
      return new RSLLiteral(*n);
    else if ((v = dynamic_cast<const RSLVariable*>(this))) {
      std::map<std::string, std::string>::iterator it = vars.find(v->Var());
      return new RSLLiteral((it != vars.end()) ? it->second : "", v->Location());
    }
    else if ((c = dynamic_cast<const RSLConcat*>(this))) {
      RSLValue *left = c->Left()->Evaluate(vars, parsing_result);
      if (!left) {
        return NULL;
      }
      RSLValue *right = c->Right()->Evaluate(vars, parsing_result);
      if (!right) {
        delete left;
        return NULL;
      }
      RSLLiteral *nleft = dynamic_cast<RSLLiteral*>(left);
      if (!nleft) {
        parsing_result.SetFailure();
        parsing_result.AddError(JobDescriptionParsingError(IString("Left operand for RSL concatenation does not evaluate to a literal").str(), c->Location()));
        delete left;
        delete right;
        return NULL;
      }
      RSLLiteral *nright = dynamic_cast<RSLLiteral*>(right);
      if (!nright) {
        parsing_result.SetFailure();
        parsing_result.AddError(JobDescriptionParsingError(IString("Right operand for RSL concatenation does not evaluate to a literal").str(), c->Location()));
        delete left;
        delete right;
        return NULL;
      }
      RSLLiteral *result = new RSLLiteral(nleft->Value() + nright->Value(), left->Location());
      delete left;
      delete right;
      return result;
    }
    else if ((l = dynamic_cast<const RSLList*>(this))) {
      RSLList *result = new RSLList(l->Location());
      for (std::list<RSLValue*>::const_iterator it = l->begin();
           it != l->end(); it++) {
        RSLValue *value = (*it)->Evaluate(vars, parsing_result);
        if (!value) {
          delete result;
          return NULL;
        }
        result->Add(value);
      }
      return result;
    }
    else if ((s = dynamic_cast<const RSLSequence*>(this))) {
      RSLList *result = new RSLList(s->Location());
      for (std::list<RSLValue*>::const_iterator it = s->begin();
           it != s->end(); it++) {
        RSLValue *value = (*it)->Evaluate(vars, parsing_result);
        if (!value) {
          delete result;
          return NULL;
        }
        result->Add(value);
      }
      return new RSLSequence(result, s->Location());
    }

    return NULL;
  }

  void RSLLiteral::Print(std::ostream& os) const {
    std::string s(str);
    std::string::size_type pos = 0;
    while ((pos = s.find('"', pos)) != std::string::npos) {
      s.insert(pos, 1, '"');
      pos += 2;
    }
    os << '"' << s << '"';
  }

  void RSLVariable::Print(std::ostream& os) const {
    os << "$(" << var << ')';
  }

  RSLConcat::~RSLConcat() {
    delete left;
    delete right;
  }

  void RSLConcat::Print(std::ostream& os) const {
    os << *left << " # " << *right;
  }

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

  void RSLSequence::Print(std::ostream& os) const {
    os << "( " << *seq << " )";
  }

  RSL* RSL::Evaluate(JobDescriptionParserPluginResult& parsing_result) const {
    const RSLBoolean *b = dynamic_cast<const RSLBoolean*>(this);
    if (b && (b->Op() == RSLMulti)) {
      RSLBoolean *result = new RSLBoolean(RSLMulti);
      for (std::list<RSL*>::const_iterator it = b->begin();
           it != b->end(); it++) {
        RSL *rsl = (*it)->Evaluate(parsing_result);
        if (!rsl) {
          return NULL;
        }
        result->Add(rsl);
      }
      return result;
    }
    else {
      std::map<std::string, std::string> vars;
      return Evaluate(vars, parsing_result);
    }
  }

  RSL* RSL::Evaluate(std::map<std::string, std::string>& vars, JobDescriptionParserPluginResult& parsing_result) const {
    const RSLBoolean *b;
    const RSLCondition *c;
    if ((b = dynamic_cast<const RSLBoolean*>(this))) {
      if (b->Op() == RSLMulti) {
        parsing_result.SetFailure();
        parsing_result.AddError(JobDescriptionParsingError(IString("Multi-request operator only allowed at top level").str(), b->OpLocation()));
        return NULL;
      }
      else {
        RSLBoolean *result = new RSLBoolean(b->Op());
        std::map<std::string, std::string> vars2(vars);
        for (std::list<RSL*>::const_iterator it = b->begin();
             it != b->end(); it++) {
          RSL *rsl = (*it)->Evaluate(vars2, parsing_result);
          if (!rsl) {
            return NULL;
          }
          result->Add(rsl);
        }
        return result;
      }
    }
    else if ((c = dynamic_cast<const RSLCondition*>(this))) {
      RSLList *l = new RSLList(c->List().Location());
      if (c->Attr() == "rslsubstitution") // Underscore, in 'rsl_substitution', is removed by normalization.
        for (std::list<RSLValue*>::const_iterator it = c->begin();
             it != c->end(); it++) {
          const RSLSequence *s = dynamic_cast<const RSLSequence*>(*it);
          if (!s) {
            parsing_result.SetFailure();
            parsing_result.AddError(JobDescriptionParsingError(IString("RSL substitution is not a sequence").str(), (**it).Location())); // TODO: The term sequence is not defined in the xRSL manual.
            delete l;
            return NULL;
          }
          if (s->size() != 2) {
            parsing_result.SetFailure();
            parsing_result.AddError(JobDescriptionParsingError(IString("RSL substitution sequence is not of length 2").str(), (**it).Location()));
            delete l;
            return NULL;
          }
          std::list<RSLValue*>::const_iterator it2 = s->begin();
          RSLValue *var = (*it2)->Evaluate(vars, parsing_result);
          if (!var) {
            delete l;
            return NULL;
          }
          it2++;
          RSLValue *val = (*it2)->Evaluate(vars, parsing_result);
          if (!val) {
            delete l;
            return NULL;
          }
          RSLLiteral *nvar = dynamic_cast<RSLLiteral*>(var);
          if (!nvar) {
            parsing_result.SetFailure();
            parsing_result.AddError(JobDescriptionParsingError(IString("RSL substitution variable name does not evaluate to a literal").str(), var->Location()));
            delete l;
            delete var;
            delete val;
            return NULL;
          }
          RSLLiteral *nval = dynamic_cast<RSLLiteral*>(val);
          if (!nval) {
            parsing_result.SetFailure();
            parsing_result.AddError(JobDescriptionParsingError(IString("RSL substitution variable value does not evaluate to a literal").str(), val->Location()));
            delete l;
            delete var;
            delete val;
            return NULL;
          }
          vars[nvar->Value()] = nval->Value();
          RSLList *seq = new RSLList(l->Location());
          seq->Add(var);
          seq->Add(val);
          l->Add(new RSLSequence(seq, s->Location()));
        }
      else
        for (std::list<RSLValue*>::const_iterator it = c->begin();
             it != c->end(); it++) {
          RSLValue *v = (*it)->Evaluate(vars, parsing_result);
          if (!v) {
            delete l;
            return NULL;
          }
          l->Add(v);
        }
      return new RSLCondition(c->Attr(), c->Op(), l);
    }

    return NULL;
  }

  RSLBoolean::~RSLBoolean() {
    for (std::list<RSL*>::iterator it = begin(); it != end(); it++)
      delete *it;
  }

  void RSLBoolean::Add(RSL *condition) {
    conditions.push_back(condition);
  }

  void RSLBoolean::Print(std::ostream& os) const {
    os << op.v;
    for (std::list<RSL*>::const_iterator it = begin(); it != end(); it++)
      os << "( " << **it << " )";
  }

  void RSLCondition::init() {
    // Normalize the attribute name
    // Does the same thing as globus_rsl_assist_attributes_canonicalize,
    // i.e. lowercase the attribute name and remove underscores
    this->attr.v = lower(this->attr.v);
    std::string::size_type pos = 0;
    while ((pos = this->attr.v.find('_', pos)) != std::string::npos) {
      this->attr.v.erase(pos, 1);
    }
  }

  void RSLCondition::Print(std::ostream& os) const {
    os << attr.v << ' ' << op.v << ' ' << *values;
  }

  RSLParser::~RSLParser() {
    if (parsed)
      delete parsed;
    if (evaluated)
      delete evaluated;
  }

  std::pair<int, int> RSLParser::GetLinePosition(std::string::size_type pos) const {
    if (pos > s.size()) {
      return std::pair<int, int>(-1, -1);
    }

    std::pair<int, int> line_pos(1, pos);
    std::string::size_type nl_pos, offset = 0;
    while ((nl_pos = s.find_first_of('\n', offset)) < pos) {
      line_pos.first += 1;
      line_pos.second = pos-nl_pos-1;
      offset = nl_pos+1;
    }
    return line_pos;
  }

  template<class T>
  SourceLocation<T> RSLParser::toSourceLocation(const T& v, std::string::size_type offset) const {
    return SourceLocation<T>(GetLinePosition(n-offset), v);
  }

  const RSL* RSLParser::Parse(bool evaluate) {
    if (n == 0) {
      std::string::size_type pos = 0;
      while ((pos = s.find("(*", pos)) != std::string::npos) {
        std::string::size_type pos2 = s.find("*)", pos);
        if (pos2 == std::string::npos) {
          int failing_code_start = std::max<int>(pos-10, 0);
          const std::string failing_code = s.substr(failing_code_start, pos-failing_code_start+12);
          parsing_result.AddError(JobDescriptionParsingError(IString("End of comment not found").str(), GetLinePosition(pos+2), failing_code));
          return NULL;
        }
        comments_positions[pos] = pos2+2;
        pos = pos2+2;
      }
      parsed = ParseRSL();
      if (parsed) {
        SkipWSAndComments();
        if (n != std::string::npos) {
          parsing_result.SetFailure();
          parsing_result.AddError(JobDescriptionParsingError(IString("Junk at end of RSL").str(), GetLinePosition(n)));
          delete parsed;
          parsed = NULL;
          return NULL;
        }
      }
      if (parsed) {
        evaluated = parsed->Evaluate(parsing_result);
      }

      if ((!evaluate && parsed) || (evaluate && evaluated)) {
        parsing_result.SetSuccess();
      }
    }
    return evaluate ? evaluated : parsed;
  }

  void RSLParser::SkipWSAndComments() {
    std::string::size_type prev_n = std::string::npos;
    while (prev_n != n) {
      prev_n = n;
      n = s.find_first_not_of(" \t\n\v\f\r", n);
      std::map<std::string::size_type, std::string::size_type>::const_iterator it = comments_positions.find(n);
      if (it != comments_positions.end()) {
        n = it->second;
      }
    }
  }

  SourceLocation<RSLBoolOp> RSLParser::ParseBoolOp() {
    switch (s[n]) {
    case '+':
      n++;
      return toSourceLocation(RSLMulti);
      break;

    case '&':
      n++;
      return toSourceLocation(RSLAnd);
      break;

    case '|':
      n++;
      return toSourceLocation(RSLOr);
      break;

    default:
      return toSourceLocation(RSLBoolError, 0);
      break;
    }
    return toSourceLocation(RSLBoolError, 0); // to keep compiler happy
  }

  SourceLocation<RSLRelOp> RSLParser::ParseRelOp() {
    switch (s[n]) {
    case '=':
      n++;
      return toSourceLocation(RSLEqual);
      break;

    case '!':
      if (s[n + 1] == '=') {
        n += 2;
        return toSourceLocation(RSLNotEqual, 2);
      }
      return toSourceLocation(RSLRelError, 0);
      break;

    case '<':
      n++;
      if (s[n] == '=') {
        n++;
        return toSourceLocation(RSLLessOrEqual, 2);
      }
      return toSourceLocation(RSLLess);
      break;

    case '>':
      n++;
      if (s[n] == '=') {
        n++;
        return toSourceLocation(RSLGreaterOrEqual, 2);
      }
      return toSourceLocation(RSLGreater);
      break;

    default:
      return toSourceLocation(RSLRelError, 0);
      break;
    }
    return toSourceLocation(RSLRelError, 0); // to keep compiler happy
  }

  SourceLocation<std::string> RSLParser::ParseString(int& status) {
    // status: 1 - OK, 0 - not a string, -1 - error
    if (s[n] == '\'') {
      SourceLocation<std::string> str(toSourceLocation(std::string(), 0));
      do {
        std::string::size_type pos = s.find('\'', n + 1);
        if (pos == std::string::npos) {
          parsing_result.AddError(JobDescriptionParsingError(IString("End of single quoted string not found").str(), GetLinePosition(n)));
          status = -1;
          return toSourceLocation(std::string(), 0);
        }
        str += s.substr(n + 1, pos - n - 1);
        n = pos + 1;
        if (s[n] == '\'')
          str += std::string("\'");
      } while (s[n] == '\'');
      status = 1;
      return str;
    }
    else if (s[n] == '"') {
      SourceLocation<std::string> str(toSourceLocation(std::string(), 0));
      do {
        std::string::size_type pos = s.find('"', n + 1);
        if (pos == std::string::npos) {
          parsing_result.AddError(JobDescriptionParsingError(IString("End of double quoted string not found").str(), GetLinePosition(n)));
          status = -1;
          return toSourceLocation(std::string(), 0);
        }
        str += s.substr(n + 1, pos - n - 1);
        n = pos + 1;
        if (s[n] == '"')
          str += std::string("\"");
      } while (s[n] == '"');
      status = 1;
      return str;
    }
    else if (s[n] == '^') {
      n++;
      char delim = s[n];
      SourceLocation<std::string> str(toSourceLocation(std::string(), 0));
      do {
        std::string::size_type pos = s.find(delim, n + 1);
        if (pos == std::string::npos) {
          parsing_result.AddError(JobDescriptionParsingError(IString("End of user delimiter (%s) quoted string not found", delim).str(), GetLinePosition(n)));
          status = -1;
          return toSourceLocation(std::string(), 0);
        }
        str += s.substr(n + 1, pos - n - 1);
        n = pos + 1;
        if (s[n] == delim)
          str += std::string(1, delim);
      } while (s[n] == delim);
      status = 1;
      return str;
    }
    else {
      std::string::size_type pos =
        s.find_first_of("+&|()=<>!\"'^#$ \t\n\v\f\r", n);
      if (pos == n) {
        status = 0;
        return toSourceLocation(std::string(), 0);
      }
      SourceLocation<std::string> str = s.substr(n, pos - n);
      str.location = GetLinePosition(n);
      n = pos;
      status = 1;
      return str;
    }
  }

  RSLList* RSLParser::ParseList() {

    RSLList *values = new RSLList(GetLinePosition(n));
    RSLValue *left = NULL;
    RSLValue *right = NULL;

    try {
      int concat = 0; // 0 = No, 1 = Explicit, 2 = Implicit
      std::pair<int, int> concatLocation;
      do {
        right = NULL;
        int nextconcat = 0;
        std::string::size_type nsave = n;
        SkipWSAndComments(); // TODO: Skipping comments increases n - not compatible with earlier approach.
        if (n != nsave)
          concat = 0;
        if (s[n] == '#') {
          concatLocation = GetLinePosition(n);
          n++;
          SkipWSAndComments();
          concat = 1;
        }
        if (concat == 2) {
          concatLocation = GetLinePosition(n);
        }
        if (s[n] == '(') {
          std::pair<int, int> seqLocation = GetLinePosition(n);
          n++;
          RSLList *seq = ParseList();
          SkipWSAndComments();
          if (s[n] != ')') {
            parsing_result.AddError(JobDescriptionParsingError(IString("')' expected").str(), GetLinePosition(n)));
            throw std::exception();
          }
          n++;
          right = new RSLSequence(seq, seqLocation);
        }
        else if (s[n] == '$') {
          n++;
          SkipWSAndComments();
          if (s[n] != '(') {
            parsing_result.AddError(JobDescriptionParsingError(IString("'(' expected").str(), GetLinePosition(n)));
            throw std::exception();
          }
          n++;
          SkipWSAndComments();
          int status;
          SourceLocation<std::string> var = ParseString(status);
          if (status != 1) {
            parsing_result.AddError(JobDescriptionParsingError(IString("Variable name expected").str(), GetLinePosition(n)));
            throw std::exception();
          }
          const std::string invalid_var_chars = "+&|()=<>!\"'^#$";
          if (var.v.find_first_of(invalid_var_chars) != std::string::npos) {
            parsing_result.AddError(JobDescriptionParsingError(IString("Variable name (%s) contains invalid character (%s)", var.v, invalid_var_chars).str(), GetLinePosition(n)));
            throw std::exception();
          }
          SkipWSAndComments();
          if (s[n] != ')') {
            parsing_result.AddError(JobDescriptionParsingError(IString("')' expected").str(), GetLinePosition(n)));
            throw std::exception();
          }
          n++;
          right = new RSLVariable(var);
          nextconcat = 2;
        }
        else {
          int status;
          SourceLocation<std::string> val = ParseString(status);
          if (status == -1) {
            parsing_result.AddError(JobDescriptionParsingError(IString("Broken string").str(), GetLinePosition(n)));
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
            parsing_result.AddError(JobDescriptionParsingError(IString("No left operand for concatenation operator").str(), GetLinePosition(n)));
            throw std::exception();
          }
          if (!right) {
            parsing_result.AddError(JobDescriptionParsingError(IString("No right operand for concatenation operator").str(), GetLinePosition(n)));
            throw std::exception();
          }
          left = new RSLConcat(left, right, concatLocation);
        }
        else if (concat == 2) {
          if (left) {
            if (right)
              left = new RSLConcat(left, right, concatLocation);
          }
          else
            left = right;
        }
        concat = nextconcat;
      } while (left || right);
    } catch (std::exception& e) {
      if (values)
        delete values;
      if (left)
        delete left;
      if (right)
        delete right;
      return NULL;
    }
    return values;
  }

  RSL* RSLParser::ParseRSL() {
    SkipWSAndComments();
    SourceLocation<RSLBoolOp> bop(ParseBoolOp());
    if (bop != RSLBoolError) {
      SkipWSAndComments();
      RSLBoolean *b = new RSLBoolean(bop);
      do {
        if (s[n] != '(') {
          parsing_result.AddError(JobDescriptionParsingError(IString("'(' expected").str(), GetLinePosition(n)));
          delete b;
          return NULL;
        }
        n++;
        SkipWSAndComments();
        RSL *rsl = ParseRSL();
        if (!rsl) {
          delete b;
          return NULL;
        }
        // Something was parsed (rsl recognised) - change intermediate parsing result (default WrongLanguage) - set to Failure - if parsing succeeds result is changed at end of parsing.
        parsing_result.SetFailure();
        b->Add(rsl);
        SkipWSAndComments();
        if (s[n] != ')') {
          parsing_result.AddError(JobDescriptionParsingError(IString("')' expected").str(), GetLinePosition(n)));
          delete b;
          return NULL;
        }
        n++;
        SkipWSAndComments();
      } while (n < s.size() && s[n] == '(');
      return b;
    }
    else {
      int status;
      SourceLocation<std::string> attr(ParseString(status));
      if (status != 1) {
        parsing_result.AddError(JobDescriptionParsingError(IString("Attribute name expected").str(), GetLinePosition(n)));
        return NULL;
      }
      const std::string invalid_attr_chars = "+&|()=<>!\"'^#$";
      if (attr.v.find_first_of(invalid_attr_chars) != std::string::npos) {
        parsing_result.AddError(JobDescriptionParsingError(IString("Attribute name (%s) contains invalid character (%s)", attr.v, invalid_attr_chars).str(), GetLinePosition(n)));
        return NULL;
      }
      SkipWSAndComments();
      SourceLocation<RSLRelOp> rop = ParseRelOp();
      if (rop == RSLRelError) {
        parsing_result.AddError(JobDescriptionParsingError(IString("Relation operator expected").str(), GetLinePosition(n)));
        return NULL;
      }
      SkipWSAndComments();
      RSLList *values = ParseList();
      if (!values) {
        return NULL;
      }
      RSLCondition *c = new RSLCondition(attr, rop, values);
      return c;
    }
  }

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

    return os;
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

    return os;
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
