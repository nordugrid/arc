#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/StringConv.h>
#include <arc/client/ExecutionTarget.h>

#include "SoftwareVersion.h"

namespace Arc {

  Logger SoftwareVersion::logger(Logger::getRootLogger(), "SoftwareVersion");
  Logger SoftwareRequirement::logger(Logger::getRootLogger(), "SoftwareRequirement");

SoftwareVersion::SoftwareVersion(const std::string& name) : version(name) {
  tokenize(name, tokenizedVersion, "-.");
}

SoftwareVersion::SoftwareVersion(const std::string& name, const std::string& version)
  : version(name + "-" + version) {
  tokenize(version, tokenizedVersion, "-.");
}

SoftwareVersion& SoftwareVersion::operator=(const SoftwareVersion& sv) {
  version = sv.version;
  tokenizedVersion = sv.tokenizedVersion;
  return *this;
}

SoftwareVersion& SoftwareVersion::operator=(const std::string& sv) {
  version = sv;
  tokenize(sv, tokenizedVersion);
  return *this;
}

// Does not support extra 0.0.0...
bool SoftwareVersion::operator>(const SoftwareVersion& sv) const {
  int thisInt, svInt;
  bool res = false;

  std::list<std::string>::const_iterator thisIt, svIt;
  for (thisIt  = tokenizedVersion.begin(), svIt  = sv.tokenizedVersion.begin();
       thisIt != tokenizedVersion.end() && svIt != sv.tokenizedVersion.end();
       thisIt++, svIt++) {
    if (*thisIt == *svIt)
      continue;
    if (stringto(*thisIt, thisInt) && stringto(*svIt, svInt)) {
      if (thisInt > svInt) {
        res = true;
        continue;
      }
      if (thisInt == svInt)
        continue;
      if (res) // Less than. But a higher parent version number already detected.
        continue;
    }

    logger.msg(DEBUG, "String parts are not equal, and are not a number.");
    
    return false;
  }

  // No more elements to parse, return res.
  if (sv.tokenizedVersion.size() == tokenizedVersion.size()) {
    logger.msg(DEBUG, "The versions consist of the same number of parts. Returning %d.", res);
    return res;
  }

  // Left side contains extra elements. These must not be strings.
  for (; thisIt != tokenizedVersion.end(); thisIt++) {
    if (!stringto(*thisIt, thisInt)) {
      logger.msg(DEBUG, "Left side contains extra element which are not numbers.");
      return false;
    }
    
    res = true;
  }

  // Right side contains extra elements. These must not be strings.
  for (; svIt != sv.tokenizedVersion.end(); svIt++) {
    if (!stringto(*svIt, svInt)) {
      logger.msg(DEBUG, "Right side contains extra element which are not numbers.");
      return false;
    }
  }

  logger.msg(DEBUG, "lhs > rhs TRUE");

  return res;
}

bool SoftwareVersion::operator<(const SoftwareVersion& sv) const {
  int thisInt, svInt;
  bool res = false;

  std::list<std::string>::const_iterator thisIt, svIt;
  for (thisIt =  tokenizedVersion.begin(), svIt  = sv.tokenizedVersion.begin();
       thisIt != tokenizedVersion.end() && svIt != sv.tokenizedVersion.end();
       thisIt++, svIt++) {
    if (*thisIt == *svIt)
      continue;
    if (stringto(*thisIt, thisInt) && stringto(*svIt, svInt)) {
      if (thisInt < svInt) {
        res = true;
        continue;
      }
      if (thisInt == svInt)
        continue;
      if (res) // Less than. But a higher parent version number already detected.
        continue;
    }

    logger.msg(DEBUG, "String parts are not equal, and are not a number.");
    
    return false;
  }

  // No more elements to parse, return res.
  if (sv.tokenizedVersion.size() == tokenizedVersion.size()) {
    logger.msg(DEBUG, "The versions consist of the same number of parts. Returning %d.", res);
    return res;
  }

  // Left side contains extra elements. These must not be strings.
  for (; thisIt != tokenizedVersion.end(); thisIt++) {
    if (!stringto(*thisIt, thisInt)) {
      logger.msg(DEBUG, "Left side contains extra element which are not numbers.");
      return false;
    }
  }

  // Right side contains extra elements. These must not be strings.
  for (; svIt != sv.tokenizedVersion.end(); svIt++) {
    if (!stringto(*svIt, svInt)) {
      logger.msg(DEBUG, "Right side contains extra element which are not numbers.");
      return false;
    }

    res = true;
  }
  
  logger.msg(DEBUG, "lhs < rhs TRUE");
  
  return res;
}

SoftwareRequirement::SoftwareRequirement(const SoftwareVersion& sv,
                                         SVComparisonOperator svComOp,
                                         bool requiresAll)
  : requiresAll(requiresAll) {
  versions.push_back(SVComparison(sv, svComOp));
}

SoftwareRequirement::SoftwareRequirement(const SoftwareVersion& sv,
                                         SoftwareVersion::ComparisonOperator co,
                                         bool requiresAll)
  : requiresAll(requiresAll) {
  switch (co) {
  case SoftwareVersion::EQUAL:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator==));
    break;
  case SoftwareVersion::NOTEQUAL:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator!=));
    break;
  case SoftwareVersion::GREATERTHAN:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator>));
    break;
  case SoftwareVersion::LESSTHAN:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator<));
    break;
  case SoftwareVersion::GREATERTHANOREQUAL:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator>=));
    break;
  case SoftwareVersion::LESSTHANOREQUAL:
    versions.push_back(SVComparison(sv, &SoftwareVersion::operator<=));
    break;
  };
}

SoftwareRequirement& SoftwareRequirement::operator=(const SoftwareRequirement& sr) {
  requirements = sr.requirements;
  requiresAll = sr.requiresAll;
  versions = sr.versions;
  return *this;
}

bool SoftwareRequirement::isSatisfied(const std::list<SoftwareVersion>& svList) const {
  // Compare SoftwareVersion objects in the 'versions' list with those in 'svList'.
  for (std::list<SVComparison>::const_iterator it = versions.begin();
       it != versions.end(); it++) {
    // Loop over 'svList'.
    std::list<SoftwareVersion>::const_iterator itSVList = svList.begin();
    for (; itSVList != svList.end(); itSVList++) {
      if ((it->first.*(it->second))(*itSVList)) { // One of the requirements satisfied.
        logger.msg(DEBUG, "Requirement satisfied. %s %s.", it->first(), (*itSVList)());
        if (!requiresAll) // Only one satisfied requirement is needed.
          return true;
        break;
      }
    }

    if (requiresAll && // All requirements have to be satisfied.
        itSVList == svList.end()) { // End of Software list reached, ie. requirement not satisfied.
      logger.msg(DEBUG, "End of list reached requirement not met.");
      return false;
    }
  }

  // Loop over Sub-requirements.
  for (std::list<SoftwareRequirement>::const_iterator it = requirements.begin();
       it != requirements.end(); it++) {
    if (requiresAll) {
      if (!it->isSatisfied(svList)) {
        logger.msg(DEBUG, "Requirements in sub-requirements not satisfied.");
        return false;
      }
    }
    else if (it->isSatisfied(svList)) {
        logger.msg(DEBUG, "Requirements in sub-requirements satisfied.");
        return true;
      }
  }

  if (requiresAll) 
    logger.msg(DEBUG, "Requirements satisfied.");
  else
    logger.msg(DEBUG, "Requirements not satisfied.");

  return requiresAll;
}

bool SoftwareRequirement::isSatisfied(const std::list<ApplicationEnvironment>& svList) const {
  return isSatisfied(reinterpret_cast< const std::list<SoftwareVersion>& >(svList));
}

std::list<SoftwareVersion> SoftwareRequirement::getVersions(void) const {
  std::list<SoftwareVersion> v;
  for (std::list<SVComparison>::const_iterator it = versions.begin();
       it != versions.end(); it++) {
    v.push_back(it->first);
  }
  return v;
}


} // namespace Arc
