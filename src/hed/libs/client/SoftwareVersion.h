#ifndef __ARC_SOFTWAREVERSION_H__
#define __ARC_SOFTWAREVERSION_H__

#include <list>
#include <string>
#include <ostream>

#include <arc/Logger.h>

namespace Arc {

  class ApplicationEnvironment;

  class SoftwareVersion {
  public:
    SoftwareVersion() {};
    SoftwareVersion(const std::string& name);
    SoftwareVersion(const std::string& name, const std::string& version);
    SoftwareVersion(const SoftwareVersion& sv) : version(sv.version), tokenizedVersion(sv.tokenizedVersion) {};

    SoftwareVersion& operator=(const SoftwareVersion& sv);
    SoftwareVersion& operator=(const std::string& sv);

    enum ComparisonOperator {
      NOTEQUAL = 0,
      EQUAL = 1,
      GREATERTHAN = 2,
      LESSTHAN = 3,
      GREATERTHANOREQUAL = 4,
      LESSTHANOREQUAL = 5
    };

    bool operator==(const std::string& sv) const { return version == sv; }
    bool operator!=(const std::string& sv) const { return version != sv; }
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
    operator std::string(void) const { return version; }

  protected:
    std::string version;
    std::list<std::string> tokenizedVersion;

    static Logger logger;
  };

  typedef bool (SoftwareVersion::*SVComparisonOperator)(const SoftwareVersion&) const;

  class SoftwareRequirement {
  public:
    SoftwareRequirement(bool requiresAll = true) : requiresAll(requiresAll) {}
#ifndef SWIG
    SoftwareRequirement(const SoftwareVersion& sv,
                        SVComparisonOperator svComOp = &SoftwareVersion::operator==,
                        bool requiresAll = true);
#endif
    SoftwareRequirement(const SoftwareVersion& sv,
                        SoftwareVersion::ComparisonOperator co,
                        bool requiresAll = true);
    SoftwareRequirement& operator=(const SoftwareRequirement& sr);

#ifndef SWIG
    void add(const SoftwareVersion& sv, SVComparisonOperator svComOp = &SoftwareVersion::operator==) { versions.push_back(SVComparison(sv, svComOp)); }
#endif
    void add(const SoftwareRequirement& sr) { requirements.push_back(sr); }

    void setRequirement(bool all) { requiresAll = all; }
    
    bool isSatisfied(const SoftwareVersion& svList) const { return isSatisfied(std::list<SoftwareVersion>(1, svList)); }
    bool isSatisfied(const std::list<SoftwareVersion>& svList) const;
    bool isSatisfied(const std::list<ApplicationEnvironment>& svList) const;
    
    bool empty() const { return requirements.empty() && versions.empty(); }

  private:
    typedef std::pair<SoftwareVersion, SVComparisonOperator> SVComparison;
    std::list<SoftwareRequirement> requirements;
    std::list<SVComparison> versions;
    bool requiresAll;

    static Logger logger;
  };
} // namespace Arc

#endif // __ARC_SOFTWAREVERSION_H__
