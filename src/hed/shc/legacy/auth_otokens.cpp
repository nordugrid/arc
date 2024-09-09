#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <vector>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/ArcRegex.h>
#include <arc/Utils.h>

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserOTokens");

AuthResult AuthUser::match_otokens(const char* line) {
  // No need to process anything if no OTokens is present
  if(otokens_data_.empty()) return AAA_NO_MATCH;
  // parse line
  std::string subject("");
  std::string issuer("");
  std::string audience("");
  std::string scope("");
  std::string group("");
  std::string::size_type n = 0;
  n=Arc::get_token(subject,line,n," ","\"","\"");
  if((n == std::string::npos) && (subject.empty())) {
    logger.msg(Arc::ERROR, "Missing subject in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(issuer,line,n," ","\"","\"");
  if((n == std::string::npos) && (issuer.empty())) {
    logger.msg(Arc::ERROR, "Missing issuer in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(audience,line,n," ","\"","\"");
  if((n == std::string::npos) && (audience.empty())) {
    logger.msg(Arc::ERROR, "Missing audience in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(scope,line,n," ","\"","\"");
  if((n == std::string::npos) && (scope.empty())) {
    logger.msg(Arc::ERROR, "Missing scope in configuration");
    return AAA_FAILURE;
  };
  n=Arc::get_token(group,line,n," ","\"","\"");
  if((n == std::string::npos) && (group.empty())) {
    logger.msg(Arc::ERROR, "Missing group in configuration");
    return AAA_FAILURE;
  };
  logger.msg(Arc::VERBOSE, "Rule: subject: %s", subject);
  logger.msg(Arc::VERBOSE, "Rule: issuer: %s", issuer);
  logger.msg(Arc::VERBOSE, "Rule: audience: %s", audience);
  logger.msg(Arc::VERBOSE, "Rule: scope: %s", scope);
  logger.msg(Arc::VERBOSE, "Rule: group: %s", group);
  // analyse permissions
  for(std::vector<struct otokens_t>::iterator v = otokens_data_.begin();v!=otokens_data_.end();++v) {
    logger.msg(Arc::DEBUG, "Match issuer: %s", v->issuer);
    if((issuer == "*") || (issuer == v->issuer)) {
      if((subject == "*") || (subject == v->subject)) {
        if((audience == "*") || (std::find(v->audiences.begin(),v->audiences.end(),audience) != v->audiences.end())) {
          if((scope == "*") || (std::find(v->scopes.begin(),v->scopes.end(),scope) != v->scopes.end())) {
            if((group == "*") || (std::find(v->groups.begin(),v->groups.end(),group) != v->groups.end())) {
              logger.msg(Arc::VERBOSE, "Matched: %s %s %s",v->subject,v->issuer,audience,scope);
              default_otokens_ = otokens_t();
              default_otokens_.subject = v->subject;
              default_otokens_.issuer = v->issuer;
	      if(audience != "*") default_otokens_.audiences.push_back(audience);
              if(scope != "*") default_otokens_.scopes.push_back(scope);
              if(group != "*") default_otokens_.groups.push_back(group);
              return AAA_POSITIVE_MATCH;
            };
          };
        };
      };
    };
  };
  logger.msg(Arc::VERBOSE, "Matched nothing");
  return AAA_NO_MATCH;
}

namespace LogicExp {

typedef std::map< std::string,std::list<std::string> > EvalContext;

static char const * SkipWS(char const * str) {
  if(!str)
      return nullptr;
  while(*str != '\0') {
    if(!std::isspace(*str))
      return str;
    ++str;
  }
  return nullptr;
}

static char const * SkipTill(char const * str, char sep) {
  if(!str)
      return nullptr;
  while(*str != '\0') {
    if(*str == sep)
      return str;
    ++str;
  }
  return nullptr;
}

class Exception: public std::runtime_error {
public:
  Exception(char const * msg) : std::runtime_error(msg ? msg : "unknown error") {};
};

class Expression {
public:
  virtual std::string const & EvaluateValue() { throw Exception("Undefined EvaluateValue was called"); }
  virtual bool EvaluateBool(EvalContext & context) { throw Exception("Undefined EvaluateBool was called"); }
private:
  static std::string EmptyString;
};

std::string Expression::EmptyString;

class ExpressionValue: public Expression {
public:
  ExpressionValue(std::string const & value): value_(value) {}
  virtual std::string const & EvaluateValue() { return value_; }
private:
  std::string value_;
};

class ExpressionUnary: public Expression {
public:
  ExpressionUnary(char op, Expression* right): op_(op), right_(right) {}
  virtual bool EvaluateBool(EvalContext & context) { return !right_->EvaluateBool(context); }
  virtual ~ExpressionUnary() {
    delete right_;
  }
private:
  char op_;
  Expression* right_;
};

class ExpressionBinary: public Expression {
public:
  ExpressionBinary(Expression* left, char op, Expression* right): op_(op), left_(left), right_(right), regexp_(nullptr) {}
  virtual bool EvaluateBool(EvalContext & context) {
    switch(op_) {
      case '|':
        //logger.msg(Arc::DEBUG, "Evaluate operator |: left: %d",left_->EvaluateBool(context));
        //logger.msg(Arc::DEBUG, "Evaluate operator |: right: %d",right_->EvaluateBool(context));
        if(left_->EvaluateBool(context)) return true;
        if(right_->EvaluateBool(context)) return true;
	return false;
	break;
      case '&':
        //logger.msg(Arc::DEBUG, "Evaluate operator &: left: %d",left_->EvaluateBool(context));
        //logger.msg(Arc::DEBUG, "Evaluate operator &: right: %d",right_->EvaluateBool(context));
        if(!left_->EvaluateBool(context)) return false;
        if(!right_->EvaluateBool(context)) return false;
	return true;
	break;
      case '^':
        return !(left_->EvaluateBool(context) == right_->EvaluateBool(context));
	break;
      case '=':
	{
          // Left value is claim name
	  // Right value is expected claim value
          std::string lvalue = left_->EvaluateValue();
          logger.msg(Arc::DEBUG, "Evaluate operator =: left: %s",lvalue);
          logger.msg(Arc::DEBUG, "Evaluate operator =: right: %s",right_->EvaluateValue());
	  if(!lvalue.empty()) {
            EvalContext::iterator itValues = context.find(lvalue);
	    if(itValues != context.end()) {
              for(std::list<std::string>::iterator itValue = itValues->second.begin(); itValue != itValues->second.end(); ++itValue) {
                logger.msg(Arc::DEBUG, "Evaluate operator =: left from context: %s",*itValue);
                if(*itValue == right_->EvaluateValue()) return true;
	      }
	    }
	  }
          return false;
	}
	break;
      case '~':
        {
          // Left value is claim name
	  // Right value is regular expression to match
          std::string lvalue = left_->EvaluateValue();
	  if(!lvalue.empty()) {
            EvalContext::iterator itValues = context.find(lvalue);
	    if(itValues != context.end()) {
              Arc::RegularExpression regexp(right_->EvaluateValue());
              for(std::list<std::string>::iterator itValue = itValues->second.begin(); itValue != itValues->second.end(); ++itValue) {
                if(regexp.match(left_->EvaluateValue())) return true;
	      }
	    }
	  }
          return false;
        }
        break;
      default:
	break;
    }
    throw Exception((std::string("Unknown binary operation ") + op_ + " was evaluated").c_str());
  }
  virtual ~ExpressionBinary() {
    delete left_;
    delete right_;
    delete regexp_;
  }
private:
  char op_;
  Expression* left_;
  Expression* right_;
  Arc::RegularExpression* regexp_;
};



class Token {
public:
  virtual bool isValue() const { return false; }
  virtual bool isUnary() const { return false; }
  virtual bool isBinary() const { return false; }
  virtual Expression* MakeExpression() { throw Exception("Undefined value MakeExpression was called"); }
  virtual Expression* MakeExpression(Expression* right) { throw Exception("Undefined unary MakeExpression was called"); }
  virtual Expression* MakeExpression(Expression* left, Expression* right) { throw Exception("Undefined binary MakeExpression was called"); }
};

class TokenOperator: public Token {
public:
  TokenOperator(char value): value_(value) {
    logger.msg(Arc::DEBUG, "Operator token: %c",value_);
  }
  virtual bool isUnary() const { return (value_ == '!'); }
  virtual bool isBinary() const { return (value_ != '!'); }
  virtual Expression* MakeExpression(Expression* right) {
    if(!isUnary()) throw Exception("Unary MakeExpression for binary operation was called");
    return new ExpressionUnary(value_, right);
  }
  virtual Expression* MakeExpression(Expression* left, Expression* right) {
    if(!isBinary()) throw Exception("Binary MakeExpression for unary operation was called");
    return new ExpressionBinary(left, value_, right);
  }
private:
  char value_;
};

class TokenString: public Token {
public:
  static Token* Parse(char const * & str, char const * till) {
    size_t len = strcspn(str, till);
    char const * start = str;
    str+=len;
    return new TokenString(start, len);
  }
  virtual bool isValue() const { return true; }
  virtual Expression* MakeExpression() { return new ExpressionValue(value_); }

private:
  TokenString(char const * str, size_t len): value_(str,len) {
    logger.msg(Arc::DEBUG, "String token: %s",value_);
  };
  std::string value_;
};

class TokenSequence: public Token {
public:
  static Token* Parse(char const * & str, bool tillBracket = false) {
    logger.msg(Arc::DEBUG, "Sequence token parsing: %s",str);
    Arc::AutoPointer<TokenSequence> token(new TokenSequence);
    while(true) {
      str = SkipWS(str);
      if(!str) {
	if(tillBracket) throw Exception("Missing closing bracket");
        break;
      }

      char c = *str;
      if((c == ')') && tillBracket) {
        ++str;
        break;
      }

      if((c == '!') || (c == '|') || (c == '&') || (c == '^') || (c == '=') || (c == '~')) {
        token->tokens_.push_back(new TokenOperator(c));
        ++str;
      } else if(c == '(') {
        ++str;
        token->tokens_.push_back(TokenSequence::Parse(str, true));
      } else if(c == '"') {
        ++str;
        token->tokens_.push_back(TokenString::Parse(str, tillBracket?")\"":"\""));
        ++str;
      } else {
        token->tokens_.push_back(TokenString::Parse(str, tillBracket?") \t!|&^=~":" \t!|&^=~"));
      }
    }

    return token.Release();
  }

  virtual ~TokenSequence() {
    while(!tokens_.empty()) {
      Token* token = tokens_.front();
      tokens_.pop_front();
      delete token;
    }
  }

  virtual bool isValue() const { return true; }

  virtual Expression* MakeExpression() {

    std::list<Token*>::iterator itToken = tokens_.begin();
    if(itToken == tokens_.end()) throw Exception("MakeExpresion without assigned tokens was called");

    Arc::AutoPointer<Expression> exprLeft;
    {
      std::list<Token*>::iterator itTokenLeftStart(itToken);
      std::list<Token*>::iterator itTokenLeft(itToken);
      while(itTokenLeft != tokens_.end()) {
        if(!(*itTokenLeft)->isUnary()) break;
        ++itTokenLeft;
      }
      if(itTokenLeft == tokens_.end()) throw Exception("No value was found on left side of expression");
      if(!(*itTokenLeft)->isValue()) throw Exception("Left side of expression is not a value");
      itToken = itTokenLeft;
      ++itToken;
      // make expression
      exprLeft = (*itTokenLeft)->MakeExpression();
      // smash unary
      while(itTokenLeft != itTokenLeftStart) {
        Arc::AutoPointer<Expression> newExpr((*itTokenLeft)->MakeExpression(exprLeft.Ptr()));
	exprLeft.Release();
	exprLeft = newExpr;
        --itTokenLeft;
      }
    }

    while(true) {
      if(itToken == tokens_.end()) return exprLeft.Release();
      if(!(*itToken)->isBinary()) throw Exception("Binary operation was expected after procesing left side of expression");
      std::list<Token*>::iterator itTokenBinary(itToken);
      ++itToken;
      if(itToken == tokens_.end()) throw Exception("Missing right side of expression");
      
      Arc::AutoPointer<Expression> exprRight;
      {
        std::list<Token*>::iterator itTokenRightStart(itToken);
        std::list<Token*>::iterator itTokenRight(itToken);
        while(itTokenRight != tokens_.end()) {
          if(!(*itTokenRight)->isUnary()) break;
          ++itTokenRight;
        }
        if(itTokenRight == tokens_.end()) throw Exception("No value was found on right side of expression");
        if(!(*itTokenRight)->isValue()) throw Exception("Right side of expression is not a value");
        itToken = itTokenRight;
        ++itToken;
        // make expression
        exprRight = (*itTokenRight)->MakeExpression();
        // smash unary
        while(itTokenRight != itTokenRightStart) {
	  Arc::AutoPointer<Expression> newExpr((*itTokenRight)->MakeExpression(exprRight.Ptr()));
	  exprRight.Release();
	  exprRight = newExpr;
          --itTokenRight;
        }
      }

      // Join through binary operator
      Arc::AutoPointer<Expression> newExpr((*itTokenBinary)->MakeExpression(exprLeft.Ptr(), exprRight.Ptr()));
      exprLeft.Release();
      exprRight.Release();
      exprLeft = newExpr;
    }
  }

private:
  std::list<Token*> tokens_;
};

} // namespace LogicExp

AuthResult AuthUser::match_ftokens(const char* line) {
  if(otokens_data_.empty()) return AAA_NO_MATCH;
  if(!line) return AAA_NO_MATCH;
  logger.msg(Arc::DEBUG, "Matching tokens expression: %s",line);
  Arc::AutoPointer<LogicExp::Token> token(LogicExp::TokenSequence::Parse(line));
  if(!token) {
    logger.msg(Arc::DEBUG, "Failed to parse expression");
    return AAA_NO_MATCH;
  }
  Arc::AutoPointer<LogicExp::Expression> expr(token->MakeExpression());
  if(!expr) return AAA_NO_MATCH;
  for(int idx = 0; idx < otokens_data_.size(); ++idx) {
    otokens_t& token_info = otokens_data_[idx];
    std::map< std::string,std::list<std::string> >& claims = token_info.claims;
    for(std::map< std::string,std::list<std::string> >::iterator claim = claims.begin(); claim != claims.end(); ++claim) {
      std::list<std::string>& values = claim->second;
      if(values.empty()) {
        logger.msg(Arc::DEBUG, "%s: <empty>", claim->first);
      } else {
        logger.msg(Arc::DEBUG, "%s: %s", claim->first, values.front());
        for(std::list<std::string>::iterator value =  values.begin();;) {
          ++value;
          if(value == values.end()) break;
          logger.msg(Arc::DEBUG, "      %s", *value);
	}
      }
    }
    try {
      if(expr->EvaluateBool(claims)) {
        logger.msg(Arc::DEBUG, "Expression matched");
        return AAA_POSITIVE_MATCH;
      }
    } catch(std::exception& exc) {
      logger.msg(Arc::DEBUG, "Failed to evaluate expression: %s",exc.what());
      return AAA_FAILURE;
    }
  }
  logger.msg(Arc::DEBUG, "Expression failed to matched");
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

