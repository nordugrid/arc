#ifndef __ARC_SOFTWAREVERSION_H__
#define __ARC_SOFTWAREVERSION_H__

/** \file
 * \brief Software and SoftwareRequirement classes.
 */

#include <list>
#include <utility>
#include <string>
#include <ostream>

#include <arc/Logger.h>

namespace Arc {

  class ApplicationEnvironment;

  /// Used to represent software (names and version) and comparison.
  /**
   * The Software class is used to represent the name of a piece of
   * software internally. Generally software are identified by a name
   * and possibly a version number. Some software can also be
   * categorized by type or family (compilers, operating system, etc.).
   * A software object can be compared to other software objects using
   * the comparison operators contained in this class.
   * The basic usage of this class is to test if some specified software
   * requirement (SoftwareRequirement) are fulfilled, by using the
   * comparability of the class.
   *
   * Internally the Software object is represented by a family and name
   * identifier, and the software version is tokenized at the characters
   * defined in VERSIONTOKENS, and stored as a list of tokens.
   * 
   * \ingroup jobdescription
   * \headerfile Software.h arc/compute/Software.h
   */
  class Software {
  public:
    /// Definition of a comparison operator method pointer.
    /**
     * This \c typedef defines a comparison operator method pointer.
     *
     * @see #operator==,
     * @see #operator!=,
     * @see #operator>,
     * @see #operator<,
     * @see #operator>=,
     * @see #operator<=,
     * @see ComparisonOperatorEnum.
     **/
    typedef bool (Software::*ComparisonOperator)(const Software&) const;

    /// Dummy constructor.
    /**
     * This constructor creates a empty object.
     **/
    Software() : family(""), name(""), version("") {};

    /// Create a Software object.
    /**
     * Create a Software object from a single string composed of a name
     * and a version part. The created object will contain a empty
     * family part. The name and version part of the string will be
     * split at the first occurence of a dash (-) which is followed by a
     * digit (0-9). If the string does not contain such a pattern, the
     * passed string will be taken to be the name and version will be
     * empty.
     *
     * @param name_version should be a string composed of the name and
     *        version of the software to represent.
     */
    Software(const std::string& name_version);

    /// Create a Software object.
    /**
     * Create a Software object with the specified name and version.
     * The family part will be left empty.
     *
     * @param name the software name to represent.
     * @param version the software version to represent.
     **/
    Software(const std::string& name, const std::string& version);

    /// Create a Software object.
    /**
     * Create a Software object with the specified family, name and
     * version.
     *
     * @param family the software family to represent.
     * @param name the software name to represent.
     * @param version the software version to represent.
     */
    Software(const std::string& family, const std::string& name, const std::string& version);

    /// Comparison operator enum
    /**
     * The #ComparisonOperatorEnum enumeration is a 1-1 correspondance
     * between the defined comparison method operators
     * (Software::ComparisonOperator), and can be used in circumstances
     * where method pointers are not supported.
     **/
    enum ComparisonOperatorEnum {
      NOTEQUAL = 0, /**< see #operator!= */
      EQUAL = 1, /**< see #operator== */
      GREATERTHAN = 2, /**< see #operator> */
      LESSTHAN = 3, /**< see #operator< */
      GREATERTHANOREQUAL = 4, /**< see #operator>= */
      LESSTHANOREQUAL = 5 /**< see #operator<= */
    };

    /// Convert a #ComparisonOperatorEnum value to a comparison method pointer.
    /**
     * The passed #ComparisonOperatorEnum will be converted to a
     * comparison method pointer defined by the
     * Software::ComparisonOperator typedef.
     *
     * This static method is not defined in language bindings created
     * with Swig, since method pointers are not supported by Swig.
     *
     * @param co a #ComparisonOperatorEnum value.
     * @return A method pointer to a comparison method is returned.
     **/
    static ComparisonOperator convert(const ComparisonOperatorEnum& co);

    /// Indicates whether the object is empty.
    /**
     * @return \c true if the name of this object is empty, otherwise
     *         \c false.
     **/
    bool empty() const { return name.empty(); }

    /// Equality operator.
    /**
     * Two Software objects are equal only if they are of the same family,
     * have the same name and is of same version. This operator can also
     * be represented by the Software::EQUAL #ComparisonOperatorEnum
     * value.
     *
     * @param sw is the RHS Software object.
     * @return \c true when the two objects equals, otherwise \c false.
     **/
    bool operator==(const Software& sw) const { return family  == sw.family &&
                                                       name    == sw.name &&
                                                       version == sw.version; }
    /// Inequality operator.
    /**
     * The behaviour of the inequality operator is just opposite that of the
     * equality operator (operator==()).
     * 
     * @param sw is the RHS Software object.
     * @return \c true when the two objects are inequal, otherwise
     *         \c false.
     **/
    bool operator!=(const Software& sw) const { return !operator==(sw); }
     
    /// Greater-than operator.
    /**
     * For the LHS object to be greater than the RHS object they must
     * first share the same family and name. If the version of the LHS is
     * empty or the LHS and RHS versions equal then LHS is not greater than
     * RHS. If the LHS version is not empty while the RHS is then LHS is
     * greater than RHS. If both versions are non empty and not equal then,
     * the first version token of each object is compared and if they are
     * identical, the two next version tokens will be compared. If not
     * identical, the two tokens will be parsed as integers, and if parsing
     * fails the LHS is not greater than the RHS. If parsing succeeds and
     * the integers equals, the two next tokens will be compared, otherwise
     * the comparison is resolved by the integer comparison.
     *
     * If the LHS contains more version tokens than the RHS, and the
     * comparison have not been resolved at the point of equal number of
     * tokens, then if the additional tokens contains a token which
     * cannot be parsed to a integer the LHS is not greater than the
     * RHS. If the parsed integer is not 0 then the LHS is greater than
     * the RHS. If the rest of the additional tokens are 0, the LHS is
     * not greater than the RHS.
     *
     * If the RHS contains more version tokens than the LHS and comparison
     * have not been resolved at the point of equal number of tokens, or
     * simply if comparison have not been resolved at the point of equal
     * number of tokens, then the LHS is not greater than the RHS.
     *
     * @param sw is the RHS object.
     * @return \c true if the LHS is greater than the RHS, otherwise
     *         \c false.
     **/
    bool operator> (const Software& sw) const;
    /// Less-than operator.
    /**
     * The behaviour of this less-than operator is equivalent to the
     * greater-than operator (operator>()) with the LHS and RHS swapped.
     *
     * @param sw is the RHS object.
     * @return \c true if the LHS is less than the RHS, otherwise
     *         \c false.
     * @see operator>().
     **/
    bool operator< (const Software& sw) const { return sw.operator>(*this); }
    /// Greater-than or equal operator.
    /**
     * The LHS object is greater than or equal to the RHS object if
     * the LHS equal the RHS (operator==()) or if the LHS is greater
     * than the RHS (operator>()).
     *
     * @param sw is the RHS object.
     * @return \c true if the LHS is greated than or equal the RHS,
     *         otherwise \c false.
     * @see operator==(),
     * @see operator>().
     **/
    bool operator>=(const Software& sw) const { return (*this == sw ? true : *this > sw); }
    /// Less-than or equal operator.
    /**
     * The LHS object is greater than or equal to the RHS object if
     * the LHS equal the RHS (operator==()) or if the LHS is greater
     * than the RHS (operator>()).
     *
     * @param sw is the RHS object.
     * @return \c true if the LHS is less than or equal the RHS,
     *         otherwise \c false.
     * @see operator==(),
     * @see operator<().
     **/
    bool operator<=(const Software& sw) const { return (*this == sw ? true : *this < sw); }

    /// Write Software string representation to a std::ostream.
    /**
     * Write the string representation of a Software object to a
     * std::ostream.
     *
     * @param out is a std::ostream to write the string representation
     *        of the Software object to.
     * @param sw is the Software object to write to the std::ostream.
     * @return The passed std::ostream \a out is returned.
     **/
    friend std::ostream& operator<<(std::ostream& out, const Software& sw) { out << sw(); return out; }

    /// Get string representation.
    /**
     * Returns the string representation of this object, which is
     * 'family'-'name'-'version'.
     *
     * @return The string representation of this object is returned.
     * @see operator std::string().
     **/
    std::string operator()() const;
    /// Cast to string
    /**
     * This casting operator behaves exactly as #operator()() does. The
     * cast is used like (std::string) <software-object>.
     *
     * @see #operator()().
     **/
    operator std::string(void) const { return operator()(); }

    /// Get family.
    /**
     * @return The family the represented software belongs to is
     *         returned.
     **/
    const std::string& getFamily() const { return family; }
    /// Get name.
    /**
     * @return The name of the represented software is returned.
     **/
    const std::string& getName() const { return name; }
    /// Get version.
    /**
     * @return The version of the represented software is returned.
     **/
    const std::string& getVersion() const { return version; }

    const std::list<std::string>& getOptions() const { return option; }

    void addOption(const std::string& opt) { option.push_back(opt); }

    void addOptions(const std::list<std::string>& opts) { option.insert(option.end(),opts.begin(),opts.end()); }

    /// Convert Software::ComparisonOperator to a string.
    /**
     * This method is not available in language bindings created by
     * Swig, since method pointers are not supported by Swig.
     *
     * @param co is a Software::ComparisonOperator.
     * @return The string representation of the passed
     *         Software::ComparisonOperator is returned.
     **/
    static std::string toString(ComparisonOperatorEnum co);

    /// Tokens used to split version string.
    /**
     * This string constant specifies which tokens will be used to split
     * the version string.
     * **/
    static const std::string VERSIONTOKENS;

  private:
    std::string family;
    std::string name;
    std::string version;
    std::list<std::string> tokenizedVersion;
    std::list<std::string> option;

    static Logger logger;
  };


  /// Class used to express and resolve version requirements on software.
  /**
   * A requirement in this class is defined as a pair composed of a
   * Software object and either a Software::ComparisonOperator method
   * pointer or equally a Software::ComparisonOperatorEnum enum value.
   * A SoftwareRequirement object can contain multiple of such
   * requirements, and then it can specified if all these requirements
   * should be satisfied, or if it is enough to satisfy only one of
   * them. The requirements can be satisfied by a single Software object
   * or a list of either Software or ApplicationEnvironment objects, by
   * using the method isSatisfied(). This class also contain a number of
   * methods (selectSoftware()) to select Software objects which are
   * satisfying the requirements, and in this way resolving
   * requirements.
   * 
   * \ingroup jobdescription
   * \headerfile Software.h arc/compute/Software.h
   **/
  class SoftwareRequirement {
  public:
    /// Create a empty SoftwareRequirement object.
    /**
     * The created SoftwareRequirement object will contain no
     * requirements.
     **/
    SoftwareRequirement() {}

    /// Create a SoftwareRequirement object.
    /**
     * The created SoftwareRequirement object will contain one
     * requirement specified by the Software object \a sw, and the
     * Software::ComparisonOperatorEnum \a co.
     *
     * @param sw is the Software object of the requirement to add.
     * @param co is the Software::ComparisonOperatorEnum of the
     *        requirement to add.
     **/
    SoftwareRequirement(const Software& sw, Software::ComparisonOperatorEnum co);

    /// Assignment operator.
    /**
     * Set this object equal to that of the passed SoftwareRequirement
     * object \a sr.
     *
     * @param sr is the SoftwareRequirement object to set object equal
     *        to.
     **/
    SoftwareRequirement& operator=(const SoftwareRequirement& sr);

    /// Copy constructor
    /**
     * Create a SoftwareRequirement object from another
     * SoftwareRequirement object.
     *
     * @param sr is the SoftwareRequirement object to make a copy of.
     **/
    SoftwareRequirement(const SoftwareRequirement& sr) { *this = sr; }

    /// Add a Software object a corresponding comparion operator to this object.
    /**
     * Adds software name and version to list of requirements and
     * associates the comparison operator with it (equality by default).
     *
     * @param sw is the Software object to add as part of a requirement.
     * @param co is the Software::ComparisonOperatorEnum value
     *        to add as part of a requirement, the default enum will be
     *        Software::EQUAL.
     **/
    void add(const Software& sw, Software::ComparisonOperatorEnum co);

    /// Test if requirements are satisfied.
    /**
     * Returns \c true if the requirements are satisfied by the
     * specified Software \a sw, otherwise \c false is returned.
     *
     * @param sw is the Software which should satisfy the requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see isSatisfied(const std::list<Software>&) const,
     * @see isSatisfied(const std::list<ApplicationEnvironment>&) const,
     * @see selectSoftware(const Software&),
     * @see isResolved() const.
     **/
    bool isSatisfied(const Software& sw) const { return isSatisfied(std::list<Software>(1, sw)); }
    /// Test if requirements are satisfied.
    /**
     * Returns \c true if stored requirements are satisfied by
     * software specified in \a swList, otherwise \c false is returned.
     *
     * Note that if all requirements must be satisfied and multiple
     * requirements exist having identical name and family all these
     * requirements should be satisfied by a single Software object.
     *
     * @param swList is the list of Software objects which should be
     *        used to try satisfy the requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see isSatisfied(const Software&) const,
     * @see isSatisfied(const std::list<ApplicationEnvironment>&) const,
     * @see selectSoftware(const std::list<Software>&),
     * @see isResolved() const.
     **/
    bool isSatisfied(const std::list<Software>& swList) const { return isSatisfiedSelect(swList); }
    /// Test if requirements are satisfied.
    /**
     * This method behaves in exactly the same way as the
     * isSatisfied(const Software&) const method does.
     *
     * @param swList is the list of ApplicationEnvironment objects which
     *        should be used to try satisfy the requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see isSatisfied(const Software&) const,
     * @see isSatisfied(const std::list<Software>&) const,
     * @see selectSoftware(const std::list<ApplicationEnvironment>&),
     * @see isResolved() const.
     **/
    bool isSatisfied(const std::list<ApplicationEnvironment>& swList) const;

    /// Select software.
    /**
     * If the passed Software \a sw do not satisfy the requirements
     * \c false is returned and this object is not modified. If however
     * the Software object \a sw do satisfy the requirements \c true is
     * returned and the requirements are set to equal the \a sw Software
     * object.
     *
     * @param sw is the Software object used to satisfy requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see selectSoftware(const std::list<Software>&),
     * @see selectSoftware(const std::list<ApplicationEnvironment>&),
     * @see isSatisfied(const Software&) const,
     * @see isResolved() const.
     **/
    bool selectSoftware(const Software& sw) { return selectSoftware(std::list<Software>(1, sw)); }
    /// Select software.
    /**
     * If the passed list of Software objects \a swList do not satisfy
     * the requirements \c false is returned and this object is not
     * modified. If however the list of Software objects \a swList do
     * satisfy the requirements \c true is returned and the Software
     * objects satisfying the requirements will replace these with the
     * equality operator (Software::operator==) used as the comparator
     * for the new requirements.
     *
     * Note that if all requirements must be satisfied and multiple
     * requirements exist having identical name and family all these
     * requirements should be satisfied by a single Software object and
     * it will replace all these requirements.
     *
     * @param swList is a list of Software objects used to satisfy
     *        requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see selectSoftware(const Software&),
     * @see selectSoftware(const std::list<ApplicationEnvironment>&),
     * @see isSatisfied(const std::list<Software>&) const,
     * @see isResolved() const.
     **/
    bool selectSoftware(const std::list<Software>& swList);
    /// Select software.
    /**
     * This method behaves exactly as the
     * selectSoftware(const std::list<Software>&) method does.
     *
     * @param swList is a list of ApplicationEnvironment objects used to
     *        satisfy requirements.
     * @return \c true if requirements are satisfied, otherwise
     *         \c false.
     * @see selectSoftware(const Software&),
     * @see selectSoftware(const std::list<Software>&),
     * @see isSatisfied(const std::list<ApplicationEnvironment>&) const,
     * @see isResolved() const.
     **/
    bool selectSoftware(const std::list<ApplicationEnvironment>& swList);

    /// Indicates whether requirements have been resolved or not.
    /**
     * If specified that only one requirement has to be satisfied, then
     * for this object to be resolved it can only contain one
     * requirement and it has use the equal operator
     * (Software::operator==).
     *
     * If specified that all requirements has to be satisfied, then for
     * this object to be resolved each requirement must have a Software
     * object with a unique family/name composition, i.e. no other
     * requirements have a Software object with the same family/name
     * composition, and each requirement must use the equal operator
     * (Software::operator==).
     *
     * If this object has been resolved then \c true is returned when
     * invoking this method, otherwise \c false is returned.
     *
     * @return \c true if this object have been resolved, otherwise
     *         \c false.
     **/
    bool isResolved() const;

    /// Test if the object is empty.
    /**
     * @return \c true if this object do no contain any requirements,
     *         otherwise \c false.
     **/
    bool empty() const { return softwareList.empty(); }
    /// Clear the object.
    /**
     * The requirements in this object will be cleared when invoking
     * this method.
     **/
    void clear() { softwareList.clear(); comparisonOperatorList.clear(); }

    /// Get list of Software objects.
    /**
     * @return The list of internally stored Software objects is
     *         returned.
     * @see Software,
     * @see getComparisonOperatorList.
     **/
    const std::list<Software>& getSoftwareList() const { return softwareList; }
    /// Get list of comparison operators.
    /**
     * @return The list of internally stored comparison operators is
     *         returned.
     * @see Software::ComparisonOperatorEnum,
     * @see getSoftwareList.
     **/
    const std::list<Software::ComparisonOperatorEnum>& getComparisonOperatorList() const { return comparisonOperatorList; }

  private:
    std::list<Software> softwareList;
    std::list<Software::ComparisonOperatorEnum> comparisonOperatorList;

    bool isSatisfiedSelect(const std::list<Software>&, SoftwareRequirement* = NULL) const;

    static Logger logger;
  };
} // namespace Arc

#endif // __ARC_SOFTWAREVERSION_H__
