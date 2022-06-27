#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/StringConv.h>
#include <arc/compute/ExecutionTarget.h>

#include "Software.h"

namespace Arc {

  Logger Software::logger(Logger::getRootLogger(), "Software");
  Logger SoftwareRequirement::logger(Logger::getRootLogger(), "SoftwareRequirement");

  const std::string Software::VERSIONTOKENS = "-.";

Software::Software(const std::string& name_version) : family(""), version("") {
  std::size_t pos = 0;

  while (pos != std::string::npos) {
    // Look for dashes in the input string.
    pos = name_version.find_first_of("-", pos);
    if (pos != std::string::npos) {
      // 'name' and 'version' is defined to be separated at the first dash which
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
  : family(""), name(name), version(version) {
  tokenize(version, tokenizedVersion, VERSIONTOKENS);
}

Software::Software(const std::string& family, const std::string& name, const std::string& version)
  : family(family), name(name), version(version) {
  tokenize(version, tokenizedVersion, VERSIONTOKENS);
}

std::string Software::operator()() const {
  if (empty()) return "";
  if (family.empty() && version.empty()) return name;
  if (family.empty()) return name + "-" + version;
  if (version.empty()) return family + "-" + name;
  return family + "-" + name + "-" + version;
}

bool Software::operator>(const Software& sv) const {
  if (family != sv.family || name != sv.name ||
      version.empty() ||
      version == sv.version) {
    logger.msg(VERBOSE, "%s > %s => false", (std::string)*this, (std::string)sv);
    return false;
  }

  if (sv.version.empty()) {
    logger.msg(VERBOSE, "%s > %s => true", (std::string)*this, (std::string)sv);
    return true;
  }

  int lhsInt, rhsInt;
  std::list<std::string>::const_iterator lhsIt, rhsIt;
  for (lhsIt  = tokenizedVersion.begin(), rhsIt  = sv.tokenizedVersion.begin();
       lhsIt != tokenizedVersion.end() && rhsIt != sv.tokenizedVersion.end();
       lhsIt++, rhsIt++) {
    if (*lhsIt == *rhsIt)
      continue;
    if (stringto(*lhsIt, lhsInt) && stringto(*rhsIt, rhsInt)) {
      if (lhsInt > rhsInt) {
        logger.msg(VERBOSE, "%s > %s => true", (std::string)*this, (std::string)sv);
        return true;
      }
      if (lhsInt == rhsInt)
        continue;
    }
    else {
      logger.msg(VERBOSE, "%s > %s => false: %s contains non numbers in the version part.", (std::string)*this, (std::string)sv, (!stringto(*lhsIt, lhsInt) ? (std::string)*this : (std::string)sv));
      return false;
    }

    logger.msg(VERBOSE, "%s > %s => false", (std::string)*this, (std::string)sv);
    return false;
  }

  if (sv.tokenizedVersion.size() != tokenizedVersion.size()) {
    // Left side contains extra tokens. These must only contain numbers.
    for (; lhsIt != tokenizedVersion.end(); lhsIt++) {
      if (!stringto(*lhsIt, lhsInt)) { // Try to convert ot an integer.
        logger.msg(VERBOSE, "%s > %s => false: %s contains non numbers in the version part.", (std::string)*this, (std::string)sv, (std::string)*this);
        return false;
      }

      if (lhsInt != 0) {
        logger.msg(VERBOSE, "%s > %s => true", (std::string)*this, (std::string)sv);
        return true;
      }
    }
  }

  logger.msg(VERBOSE, "%s > %s => false", (std::string)*this, (std::string)sv);

  return false;
}

std::string Software::toString(ComparisonOperatorEnum co) {
  if (co == EQUAL) return "==";
  if (co == LESSTHAN)  return "<";
  if (co == GREATERTHAN)  return ">";
  if (co == LESSTHANOREQUAL) return "<=";
  if (co == GREATERTHANOREQUAL) return ">=";
  if (co == NOTEQUAL) return "!=";
  return "??";
}

Software::ComparisonOperator Software::convert(const Software::ComparisonOperatorEnum& co) {
  switch (co) {
  case Software::EQUAL:
    return &Software::operator==;
  case Software::NOTEQUAL:
    return &Software::operator!=;
  case Software::GREATERTHAN:
    return &Software::operator>;
  case Software::LESSTHAN:
    return &Software::operator<;
  case Software::GREATERTHANOREQUAL:
    return &Software::operator>=;
  case Software::LESSTHANOREQUAL:
    return &Software::operator<=;
  };
  return &Software::operator!=; // Avoid compilation warning
}

SoftwareRequirement::SoftwareRequirement(const Software& sw,
                                         Software::ComparisonOperatorEnum co)
  : softwareList(1, sw), comparisonOperatorList(1, co)
{}

SoftwareRequirement& SoftwareRequirement::operator=(const SoftwareRequirement& sr) {
  softwareList = sr.softwareList;
  comparisonOperatorList = sr.comparisonOperatorList;
  return *this;
}

void SoftwareRequirement::add(const Software& sw, Software::ComparisonOperatorEnum co) {
  if (!sw.empty()) {
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(co);
  }
}

bool SoftwareRequirement::isSatisfied(const std::list<ApplicationEnvironment>& swList) const {
  return isSatisfiedSelect(reinterpret_cast< const std::list<Software>& >(swList));
}

bool SoftwareRequirement::isSatisfiedSelect(const std::list<Software>& swList, SoftwareRequirement* sr) const {
  // Compare Software objects in the 'versions' list with those in 'swList'.
  std::list<Software>::const_iterator itSW = softwareList.begin();
  std::list<Software::ComparisonOperatorEnum>::const_iterator itSWC = comparisonOperatorList.begin();
  for (; itSW != softwareList.end() && itSWC != comparisonOperatorList.end(); itSW++, itSWC++) {
    Software * currentSelectedSoftware = NULL; // Pointer to the current selected software from the argument list.
    // Loop over 'swList'.
    std::list<Software>::const_iterator itSWList = swList.begin();
    for (; itSWList != swList.end(); itSWList++) {
      Software::ComparisonOperator op = itSWList->convert(*itSWC);
      if (((*itSWList).*op)(*itSW)) { // One of the requirements satisfied.
        if (*itSWC == Software::NOTEQUAL) {
          continue;
        }
        
        if (sr != NULL) {
          if (currentSelectedSoftware == NULL) { // First software to satisfy requirement. Push it to the selected software.
            sr->softwareList.push_back(*itSWList);
            sr->comparisonOperatorList.push_back(Software::EQUAL);
          }
          else if (*currentSelectedSoftware < *itSWList) { // Select the software with the highest version still satisfying the requirement.
            sr->softwareList.back() = *itSWList;
          }
  
          currentSelectedSoftware = &sr->softwareList.back();
        }
        else {
          break;
        }
      }
      else if (*itSWC == Software::NOTEQUAL) {
        logger.msg(VERBOSE, "Requirement \"%s %s\" NOT satisfied.", Software::toString(*itSWC), (std::string)*itSW);
        return false;
      }
    }

    if (*itSWC == Software::NOTEQUAL) {
      logger.msg(VERBOSE, "Requirement \"%s %s\" satisfied.", Software::toString(*itSWC), (std::string)*itSW);
      continue;
    }

    if (itSWList == swList.end() && currentSelectedSoftware == NULL) {
      logger.msg(VERBOSE, "Requirement \"%s %s\" NOT satisfied.", Software::toString(*itSWC), (std::string)*itSW);
      return false;
    }

    logger.msg(VERBOSE, "Requirement \"%s %s\" satisfied by \"%s\".", Software::toString(*itSWC), (std::string)*itSW, (std::string)(currentSelectedSoftware == NULL ? *itSWList : *currentSelectedSoftware));
    // Keep options from requirement
    if(currentSelectedSoftware != NULL) currentSelectedSoftware->addOptions(itSW->getOptions());
  }

  logger.msg(VERBOSE, "All requirements satisfied.");
  return true;
}

bool SoftwareRequirement::selectSoftware(const std::list<Software>& swList) {
  SoftwareRequirement* sr = new SoftwareRequirement();

  bool status = isSatisfiedSelect(swList, sr);
  
  if (status) {
    *this = *sr;
  }
  delete sr;

  return status;
}

bool SoftwareRequirement::selectSoftware(const std::list<ApplicationEnvironment>& swList) {
  return selectSoftware(reinterpret_cast< const std::list<Software>& >(swList));
}

bool SoftwareRequirement::isResolved() const {
  for (std::list<Software::ComparisonOperatorEnum>::const_iterator it = comparisonOperatorList.begin();
       it != comparisonOperatorList.end(); it++) {
    if (*it != Software::EQUAL) {
      return false;
    }
  }

  return true;
}

} // namespace Arc
