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

std::string Software::operator()() const {
  if (empty()) return "";
  if (family.empty() && version.empty()) return name;
  if (family.empty()) return name + "-" + version;
  if (version.empty()) return family + "-" + name;
  return family + "-" + name + "-" + version;
}

bool Software::operator>(const Software& sv) const {
  if (family != sv.family || name != sv.name ||
      version.empty() || sv.version.empty() ||
      version == sv.version) return false;

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
      logger.msg(VERBOSE, "%s > %s => false: \%s contains non numbers in the version part.", (std::string)*this, (std::string)sv, (!stringto(*lhsIt, lhsInt) ? (std::string)*this : (std::string)sv));
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

std::string Software::toString(ComparisonOperator co) {
  if (co == &Software::operator==) return "==";
  if (co == &Software::operator<)  return "<";
  if (co == &Software::operator>)  return ">";
  if (co == &Software::operator<=) return "<=";
  if (co == &Software::operator>=) return ">=";
  return "!=";
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
}

SoftwareRequirement::SoftwareRequirement(const Software& sw,
                                         Software::ComparisonOperatorEnum co,
                                         bool requiresAll)
  : requiresAll(requiresAll), softwareList(1, sw), comparisonOperatorList(1, Software::convert(co)),
    orderedSoftwareList(1, std::list<SWRelPair>(1, SWRelPair(&softwareList.front(), comparisonOperatorList.front())))
{}

SoftwareRequirement::SoftwareRequirement(const Software& sw,
                                         Software::ComparisonOperator swComOp,
                                         bool requiresAll)
  : softwareList(1, sw), comparisonOperatorList(1, swComOp), requiresAll(requiresAll),
    orderedSoftwareList(1, std::list<SWRelPair>(1, SWRelPair(&softwareList.front(), comparisonOperatorList.front())))
{}

SoftwareRequirement& SoftwareRequirement::operator=(const SoftwareRequirement& sr) {
  requiresAll = sr.requiresAll;
  softwareList = sr.softwareList;
  comparisonOperatorList = sr.comparisonOperatorList;

  orderedSoftwareList.clear();
  std::list<Software>::iterator itSW = softwareList.begin();
  std::list<Software::ComparisonOperator>::iterator itCO = comparisonOperatorList.begin();
  for (; itSW != softwareList.end(); itSW++, itCO++) {
    std::list< std::list<SWRelPair> >::iterator itRel = orderedSoftwareList.begin();
    for (; itRel != orderedSoftwareList.end(); itRel++) {
      if (itRel->front().first->getName() == itSW->getName() &&
          itRel->front().first->getFamily() == itSW->getFamily()) {
        itRel->push_back(SWRelPair(&*itSW, *itCO));
        break;
      }
    }

    if (itRel == orderedSoftwareList.end())
      orderedSoftwareList.push_back(std::list<SWRelPair>(1, SWRelPair(&*itSW, *itCO)));
  }

  return *this;
}


void SoftwareRequirement::add(const Software& sw, Software::ComparisonOperator swComOp) {
  if (!sw.empty()) {
    softwareList.push_back(sw);
    comparisonOperatorList.push_back(swComOp);
    for (std::list< std::list<SWRelPair> >::iterator it = orderedSoftwareList.begin();
         it != orderedSoftwareList.end(); it++) {
      if (it->front().first->getName() == sw.getName() &&
          it->front().first->getFamily() == sw.getFamily()) {
        it->push_back(SWRelPair(&softwareList.back(), comparisonOperatorList.back()));
        return;
      }
    }

    orderedSoftwareList.push_back(std::list<SWRelPair>(1, SWRelPair(&softwareList.back(), comparisonOperatorList.back())));
  }
}

void SoftwareRequirement::add(const Software& sw, Software::ComparisonOperatorEnum co) {
  add(sw, Software::convert(co));
}

bool SoftwareRequirement::isSatisfied(const std::list<Software>& swList) const {
  // Compare Software objects in the 'versions' list with those in 'swList'.
  std::list< std::list<SWRelPair> >::const_iterator itOSL = orderedSoftwareList.begin();
  for (; itOSL != orderedSoftwareList.end(); itOSL++) {
    // Loop over 'swList'.
    std::list<Software>::const_iterator itSWList = swList.begin();
    for (; itSWList != swList.end(); itSWList++) {
      std::list<SWRelPair>::const_iterator itSRL = itOSL->begin();
      for (; itSRL != itOSL->end(); itSRL++) {
        if (((*itSWList).*itSRL->second)(*itSRL->first)) { // One of the requirements satisfied.
          logger.msg(VERBOSE, "Requirement satisfied. %s %s %s.", (std::string)*itSWList, Software::toString(itSRL->second), (std::string)*itSRL->first);
          if (!requiresAll) // Only one satisfied requirement is needed.
            return true;
        }
        else {
          logger.msg(VERBOSE, "Requirement NOT satisfied. %s %s %s.", (std::string)*itSWList, Software::toString(itSRL->second), (std::string)*itSRL->first);
          if (requiresAll) // If requiresAll == true, then a element from the swList have to satisfy all requirements for a unique software (family + name).
            break;
        }
      }

      if (requiresAll && itSRL == itOSL->end()) // All requirements in the group have been satisfied by a single software.
        break;
    }

    if (requiresAll && // All requirements have to be satisfied.
        itSWList == swList.end()) { // End of Software list reached, ie. requirement not satisfied.
      logger.msg(VERBOSE, "End of list reached requirement not met.");
      return false;
    }
  }

  if (requiresAll)
    logger.msg(VERBOSE, "Requirements satisfied.");
  else
    logger.msg(VERBOSE, "Requirements not satisfied.");

  return requiresAll;
}

bool SoftwareRequirement::isSatisfied(const std::list<ApplicationEnvironment>& swList) const {
  return isSatisfied(reinterpret_cast< const std::list<Software>& >(swList));
}

bool SoftwareRequirement::selectSoftware(const std::list<Software>& swList) {
  SoftwareRequirement sr(requiresAll);

  std::list< std::list<SWRelPair> >::const_iterator itOSL = orderedSoftwareList.begin();
  for (; itOSL != orderedSoftwareList.end(); itOSL++) {
    Software * currentSelectedSoftware = NULL; // Pointer to the current selected software from the argument list.
    for (std::list<Software>::const_iterator itSWList = swList.begin();
         itSWList != swList.end(); itSWList++) {
      std::list<SWRelPair>::const_iterator itSRP = itOSL->begin();
      for (; itSRP != itOSL->end(); itSRP++) {
        if (((*itSWList).*itSRP->second)(*itSRP->first)) { // Requirement is satisfied.
          if (!requiresAll)
            break;
        }
        else if (requiresAll)
          break;
      }

      if (requiresAll && itSRP == itOSL->end() || // All requirements satisfied by this software.
          !requiresAll && itSRP != itOSL->end()) { // One requirement satisfied by this software.
        if (currentSelectedSoftware == NULL) { // First software to satisfy requirement. Push it to the selected software.
          sr.softwareList.push_back(*itSWList);
          sr.comparisonOperatorList.push_back(&Software::operator ==);
        }
        else if (*currentSelectedSoftware < *itSWList) { // Select the software with the highest version still satisfying the requirement.
          sr.softwareList.back() = *itSWList;
        }

        currentSelectedSoftware = &sr.softwareList.back();
      }
    }

    if (!requiresAll && sr.softwareList.size() == 1) { // Only one requirement need to be satisfied.
      *this = sr;
      return true;
    }

    if (requiresAll && currentSelectedSoftware == NULL)
      return false;
  }

  if (requiresAll)
    *this = sr;

  return requiresAll;
}

bool SoftwareRequirement::selectSoftware(const std::list<ApplicationEnvironment>& swList) {
  return selectSoftware(reinterpret_cast< const std::list<Software>& >(swList));
}

bool SoftwareRequirement::isResolved() const {
  if (!requiresAll)
    return softwareList.size() <= 1 && (softwareList.size() == 0 || comparisonOperatorList.front() == &Software::operator==);
  else {
    for (std::list< std::list<SWRelPair> >::const_iterator it = orderedSoftwareList.begin();
         it != orderedSoftwareList.end(); it++) {
      if (it->size() > 1 || it->front().second != &Software::operator==)
        return false;
    }

    return true;
  }
}

} // namespace Arc
