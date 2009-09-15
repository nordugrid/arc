#ifndef __ARC_SOFTWAREVERSION_H__
#define __ARC_SOFTWAREVERSION_H__

#include <list>
#include <string>
#include <ostream>

#include <arc/Logger.h>

namespace Arc {

  class ApplicationEnvironment;

  /**
   * The Software class is used to represent the name of a piece of software
   * internally. Generally software are identified by a name and possibly a
   * version number. Some software can also be categorized by type or family
   * (compilers, operating system, etc.). The basic usage of this class is to
   * test if some specified software requirement are fulfilled, by using the
   * comparability of the class.
   */
  class Software {
  public:
    /// Dummy constructor. The created object will be empty.
    Software() : family(""), name(""), version("") {};

    /**
     * Create a Software object from a single string composed of a name and a
     * version part. The object will contain a empty family part.
     * The name and version part of the string will be split at the first
     * occurence of a dash (-) which is followed by a digit (0-9). If the string
     * does not contain such a pattern, the passed string will be taken to be
     * the name and version will be empty.
     */
    Software(const std::string& name_version);

    /// Create a Software object with the specified name and version. The family part will be left empty.
    Software(const std::string& name, const std::string& version);

    /// Create a Software object with the specified family, name and version.
    Software(const std::string& family, const std::string& name, const std::string& version);

    /// Copy constructor.
    Software(const Software& sv) : family(sv.family), name(sv.name), version(sv.version), tokenizedVersion(sv.tokenizedVersion) {};

    Software& operator=(const Software& sv);

    /// The ComparisonOperator enumeration has a 1-1 correspondance between the
    /// defined comparison method operators, and can be used in situations where
    /// member function pointers are not supported.
    enum ComparisonOperator {
      NOTEQUAL = 0,
      EQUAL = 1,
      GREATERTHAN = 2,
      LESSTHAN = 3,
      GREATERTHANOREQUAL = 4,
      LESSTHANOREQUAL = 5
    };

    /// Returns true if the name part is empty, otherwise false.
    bool empty() const { return name.empty(); }

    /// Two Software objects are equal (returns true) if they are of the same family, and if they
    /// have the same name. If BOTH objects specifies a version they must also equal, for the objects to be equal.
    /// Otherwise the two objects does not equal (returns false).
    bool operator==(const Software& sv) const { return family  == sv.family &&
                                                       name    == sv.name &&
                                                       (version.empty() || sv.version.empty() || version == sv.version); }
    /// Returns the negation of the == operator.
    bool operator!=(const Software& sv) const { return !(sv == *this); }
    bool operator> (const Software& sv) const;
    bool operator< (const Software& sv) const;
    bool operator<=(const Software& sv) const { return (*this == sv ? true : *this < sv); }
    bool operator>=(const Software& sv) const { return (*this == sv ? true : *this > sv); }

    friend std::ostream& operator<<(std::ostream& out, const Software& sv) {
      out << sv();
      return out;
    }

    /// Returns the string representation of this object, which is 'family'-'name'-'version'.
    std::string operator()() const;
    operator std::string(void) const { return operator()(); }

    std::string getFamily() const { return family; }
    std::string getName() const { return name; }
    std::string getVersion() const { return version; }

#ifndef SWIG
    static std::string toString(bool (Software::*co)(const Software&) const);
#endif

    /// This string constant specifies which tokens will be used to split the version string.
    static const std::string VERSIONTOKENS;

  protected:
    std::string family;
    std::string name;
    std::string version;
    std::list<std::string> tokenizedVersion;

    static Logger logger;
  };

  typedef bool (Software::*SWComparisonOperator)(const Software&) const;

  

  class SoftwareRequirement {
  public:
    SoftwareRequirement(bool requiresAll = false) : requiresAll(requiresAll) {}
#ifndef SWIG
    SoftwareRequirement(const Software& sw,
                        SWComparisonOperator swComOp = &Software::operator==,
                        bool requiresAll = false)
      : softwareList(1, sw), comparisonOperatorList(1, swComOp), requiresAll(requiresAll) {}
#endif
    SoftwareRequirement(const Software& sw,
                        Software::ComparisonOperator co,
                        bool requiresAll = false);
    SoftwareRequirement& operator=(const SoftwareRequirement& sr);

#ifndef SWIG
    /// Adds software name and version to list of requirements and
    /// associates comparison operator with it (equality by default)
    void add(const Software& sw, SWComparisonOperator swComOp = &Software::operator==) { if (!sw.empty()) { softwareList.push_back(sw); comparisonOperatorList.push_back(swComOp); } }
#endif

    bool isRequiringAll() const { return requiresAll; }

    /// Specifies if all requirements stored need to be satisfied
    /// or if it is enough to satisfy only one.
    void setRequirement(bool all) { requiresAll = all; }

    /// Returns true if stored requirements are satisfied by
    /// software specified in swList.
    bool isSatisfied(const Software& sw) const { return isSatisfied(std::list<Software>(1, sw)); }
    bool isSatisfied(const std::list<Software>& swList) const;
    bool isSatisfied(const std::list<ApplicationEnvironment>& swList) const;

    bool selectSoftware(const Software& sw) { return selectSoftware(std::list<Software>(1, sw)); }
    bool selectSoftware(const std::list<Software>& swList);
    bool selectSoftware(const std::list<ApplicationEnvironment>& swList);
    
    bool empty() const { return softwareList.empty(); }

    /// Returns list of the contained software.
    const std::list<Software>& getSoftwareList() const { return softwareList; }
    /// Returns list of comparison operators.
    const std::list<SWComparisonOperator>& getComparisonOperatorList() const { return comparisonOperatorList; }

  private:
    std::list<Software> softwareList;
    std::list<SWComparisonOperator> comparisonOperatorList; 
    bool requiresAll;

    static Logger logger;
  };
} // namespace Arc

#endif // __ARC_SOFTWAREVERSION_H__
