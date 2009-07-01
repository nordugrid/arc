#ifndef __ARC_SOFTWAREVERSION_H__
#define __ARC_SOFTWAREVERSION_H__

#include <list>
#include <string>
#include <ostream>

#include <arc/Logger.h>

namespace Arc {

  class SoftwareVersion {
  public:
    SoftwareVersion() {};
    SoftwareVersion(const std::string& name);

    SoftwareVersion& operator=(const SoftwareVersion& sv);
    
    bool operator==(const SoftwareVersion& sv) const { return sv.version == version; }
    bool operator!=(const SoftwareVersion& sv) const { return sv.version != version; }
    bool operator> (const SoftwareVersion& sv) const;
    bool operator< (const SoftwareVersion& sv) const;
    bool operator<=(const SoftwareVersion& sv) const { return (*this == sv ? true : *this < sv); }
    bool operator>=(const SoftwareVersion& sv) const { return (*this == sv ? true : *this > sv); }

    friend std::ostream& operator<<(std::ostream& out, const SoftwareVersion& sv)
    {
      out << sv.version;
      return out;
    }

    std::string operator()() const { return version; }

  private:
    std::string version;
    std::list<std::string> tokenizedVersion;

    static Logger logger;
  };

  typedef bool (SoftwareVersion::*SVComparisonOperator)(const SoftwareVersion&) const;

  class SoftwareRequirements {
  public:
    SoftwareRequirements(bool requiresAll = true) : requiresAll(requiresAll) {}
    SoftwareRequirements(const SoftwareVersion& sv,
                         const SVComparisonOperator& svComOp = &SoftwareVersion::operator==,
                         bool requiresAll = true)
      : versions(1, SVComparison(sv, svComOp)), requiresAll(requiresAll) {}

    void add(const SoftwareVersion& sv, const SVComparisonOperator& svComOp = &SoftwareVersion::operator==) { versions.push_back(SVComparison(sv, svComOp)); }
    void add(const SoftwareRequirements& sr) { requirements.push_back(sr); }
    
    bool isSatisfied(const std::list<SoftwareVersion>& svList) const;

  private:
    typedef std::pair<SoftwareVersion, const SVComparisonOperator> SVComparison;
    std::list<SoftwareRequirements> requirements;
    std::list<SVComparison> versions;
    const bool requiresAll;

    static Logger logger;
  };
} // namespace Arc

#endif // __ARC_SOFTWAREVERSION_H__
