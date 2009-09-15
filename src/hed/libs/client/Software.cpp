#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/StringConv.h>
#include <arc/client/ExecutionTarget.h>

#include "Software.h"

namespace Arc {

  Logger Software::logger(Logger::getRootLogger(), "Software");
  Logger SoftwareRequirement::logger(Logger::getRootLogger(), "SoftwareRequirement");

  const std::string Software::VERSIONTOKENS = "-.";

Software::Software(const std::string& name_version) : version(""), family("") {
  std::size_t pos = 0;
  
  while (pos != std::string::npos) {
    // Look for dashes in the input string.
    pos = name_version.find_first_of("-", pos);
    if (pos != std::string::npos) {
      // 'name' and 'version' is defined to be seperated at the first dash which
      // is followed by a digit.
      if (isdigit(name_version[pos+1])) {
        name = name_version.substr(0, pos);
        version = name_version.substr(pos+1);
        tokenize(version, tokenizedVersion, VERSIONTOKENS);
        return;
      }

      pos++;
    }
  }

  // If no version part is found from the input string set the input to be the name.
  name = name_version;
}

Software::Software(const std::string& name, const std::string& version)
  : name(name), version(version), family("") {
  tokenize(version, tokenizedVersion, VERSIONTOKENS);
}

Software::Software(const std::string& family, const std::string& name, const std::string& version)
  : family(family), name(name), version(version) {
  tokenize(version, tokenizedVersion, VERSIONTOKENS);
}

Software& Software::operator=(const Software& sv) {
  family = sv.family;
  name = sv.name;
  version = sv.version;
  tokenizedVersion = sv.tokenizedVersion;
  return *this;
}

std::string Software::operator()() const {
  if (empty()) return "";
  if (family.empty() && version.empty()) return name;
  if (family.empty()) return name + "-" + version;
  if (version.empty()) return family + "-" + name;
  return family + "-" + name + "-" + version;
}

// Does not support extra 0.0.0...
bool Software::operator>(const Software& sv) const {
  if (family != sv.family || name != sv.name ||
      version.empty() || sv.version.empty()) return false;
  
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

bool Software::operator<(const Software& sv) const {
  if (family != sv.family || name != sv.name ||
      version.empty() || sv.version.empty()) return false;
  
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

std::string Software::toString(SWComparisonOperator co) {
  if (co == &Software::operator==) return "==";
  if (co == &Software::operator<)  return "<";
  if (co == &Software::operator>)  return ">";
  if (co == &Software::operator<=) return "<=";
  if (co == &Software::operator>=) return ">=";
  return "!=";
}

SoftwareRequirement::SoftwareRequirement(const Software& sw,
                                         Software::ComparisonOperator co,
                                         bool requiresAll)
  : requiresAll(requiresAll) {
  switch (co) {
  case Software::EQUAL:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator==);
    break;
  case Software::NOTEQUAL:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator!=);
    break;
  case Software::GREATERTHAN:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator>);
    break;
  case Software::LESSTHAN:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator<);
    break;
  case Software::GREATERTHANOREQUAL:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator>=);
    break;
  case Software::LESSTHANOREQUAL:
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(&Software::operator<=);
    break;
  };
}

SoftwareRequirement& SoftwareRequirement::operator=(const SoftwareRequirement& sr) {
  requiresAll = sr.requiresAll;
  softwareList = sr.softwareList;
  comparisonOperatorList = sr.comparisonOperatorList;
  return *this;
}

bool SoftwareRequirement::isSatisfied(const std::list<Software>& swList) const {
  // Compare Software objects in the 'versions' list with those in 'swList'.
  std::list<Software>::const_iterator itSWSelf = softwareList.begin();
  std::list<SWComparisonOperator>::const_iterator itCO = comparisonOperatorList.begin();
  for (; itSWSelf != softwareList.end(); itSWSelf++, itCO++) {
    // Loop over 'swList'.
    std::list<Software>::const_iterator itSWList = swList.begin();
    for (; itSWList != swList.end(); itSWList++) {
      if (((*itSWList).*(*itCO))(*itSWSelf)) { // One of the requirements satisfied.
        logger.msg(DEBUG, "Requirement satisfied. %s %s.", (std::string)*itSWSelf, (std::string)*itSWList);
        if (!requiresAll) // Only one satisfied requirement is needed.
          return true;
        break;
      }
    }

    if (requiresAll && // All requirements have to be satisfied.
        itSWList == swList.end()) { // End of Software list reached, ie. requirement not satisfied.
      logger.msg(DEBUG, "End of list reached requirement not met.");
      return false;
    }
  }

  if (requiresAll) 
    logger.msg(DEBUG, "Requirements satisfied.");
  else
    logger.msg(DEBUG, "Requirements not satisfied.");

  return requiresAll;
}

bool SoftwareRequirement::isSatisfied(const std::list<ApplicationEnvironment>& swList) const {
  return isSatisfied(reinterpret_cast< const std::list<Software>& >(swList));
}

bool SoftwareRequirement::selectSoftware(const std::list<Software>& swList) {
  std::list<Software> selectedSoftware;
  std::list<SWComparisonOperator> selectedSoftwareCO;
  
  std::list<Software>::iterator itSWSelf = softwareList.begin();
  std::list<SWComparisonOperator>::iterator itCO = comparisonOperatorList.begin();
  for (; itSWSelf != softwareList.end(); itSWSelf++, itCO++) {
    Software * currentSelectedSoftware = NULL; // Pointer to the current selected software from the argument list.
    
    for (std::list<Software>::const_iterator itSWList = swList.begin();
         itSWList != swList.end(); itSWList++) {
      if (((*itSWList).*(*itCO))(*itSWSelf)) { // Requirement is satisfied.
        if (currentSelectedSoftware == NULL) { // First software to satisfy requirement. Push it to the 
          selectedSoftware.push_back(*itSWList);
          selectedSoftwareCO.push_back(&Software::operator ==);
          currentSelectedSoftware = &selectedSoftware.back();
        }
        else if (*currentSelectedSoftware < *itSWList) { // Select the software with the highest version still satisfying the requirement.
          selectedSoftware.back() = *itSWList;
        }
      }
    }

    if (!requiresAll && selectedSoftware.size() == 1) { // Only one requirement need to be satisfied.
      softwareList = selectedSoftware;
      comparisonOperatorList = selectedSoftwareCO;
      return true;
    }
    
    if (requiresAll && currentSelectedSoftware == NULL)
      return false;
  }

  if (requiresAll) {
    softwareList = selectedSoftware;
    comparisonOperatorList = selectedSoftwareCO;
  }

  return requiresAll;
}

bool SoftwareRequirement::selectSoftware(const std::list<ApplicationEnvironment>& swList) {
  return selectSoftware(reinterpret_cast< const std::list<Software>& >(swList));
}

} // namespace Arc
