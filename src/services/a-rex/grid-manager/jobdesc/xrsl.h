#ifndef ARCLIB_XRSL
#define ARCLIB_XRSL

#include <list>
#include <string>

#ifdef HAVE_MEMMOVE
#include <globus_common.h>
#else
#define HAVE_MEMMOVE 1
#include <globus_common.h>
#undef HAVE_MEMMOVE
#endif

#include <globus_rsl.h>

#include "error.h"

/**
 * Class represents exceptions associated with usage of the Xrsl class.
 */
class XrslError : public ARCLibError {
	public:
		/** Standard exception constructor. */
		XrslError(std::string message_arg) : ARCLibError(message_arg) {}
};


/** Represents the operators that can be used in relations:
 * '=' ,  '!=' , '>' , '>=' , '<' , '<='
 */
enum xrsl_operator { operator_eq = GLOBUS_RSL_EQ,
                     operator_neq = GLOBUS_RSL_NEQ,
                     operator_gt = GLOBUS_RSL_GT,
                     operator_gteq = GLOBUS_RSL_GTEQ,
                     operator_lt = GLOBUS_RSL_LT,
                     operator_lteq = GLOBUS_RSL_LTEQ,
                     operator_and = GLOBUS_RSL_AND,
                     operator_or = GLOBUS_RSL_OR,
                     operator_multi = GLOBUS_RSL_MULTIREQ };


/** XrslRelation class that describes an Xrsl-relation with an attribute,
 *  an operator and a value. Various constructors and methods for extracting
 *  the attribute and the value (single value or list) are given.
 */
class XrslRelation {

	public:
		/** Constructor constructing an xrsl-relation from an attribute,
		 *  an operator and a value.
		 */
		XrslRelation(const std::string& attribute,
		             const xrsl_operator& oper,
		             const std::string& value);

		/** Constructor constructing an xrsl-relation from an attribute,
		 *  an operator and a value-list.
		 */
		XrslRelation(const std::string& attribute,
		             const xrsl_operator& oper,
		             const std::list<std::string>& value);

		/** Constructor constructing an xrsl-relation from an attribute,
		 *  an operator and a double value-list.
		 */
		XrslRelation(const std::string& attribute,
		             const xrsl_operator& oper,
		             const std::list<std::list<std::string> >& value);

		/** Constructs a relation from a globus_rsl_t*. */
		XrslRelation(globus_rsl_t* relation);

		/** Copy-constructor. */
		XrslRelation(const XrslRelation &other_relation);

		/** Destructor. */
		~XrslRelation();

		/** Returns a strng representation of the relation. */
		std::string str();

		/** Returns the attribute of the relation. */
		std::string GetAttribute() const;

		/** Returns the xrsl_operator of the attribute. */
		xrsl_operator GetOperator() const;

		/** If the value of the representation is a single string value,
		 *  return it.
		 */
		std::string GetSingleValue() throw(XrslError);

		/** If the value of the relation is a list value, return it. */
		std::list<std::string> GetListValue() throw(XrslError);

		/** If the value of the relation is a double list value, return it. */
		std::list<std::list<std::string> > GetDoubleListValue()
		throw(XrslError);

		/** Returns relation. */
		globus_rsl_t* GetRelation() const;

	private:
		/** Internal representation of relation. */
		globus_rsl_t* relation;
};


/** Relations can be either a string or a list (or lists) of strings. */
enum relation_type { RELATIONTYPE_LIST, RELATIONTYPE_STRING };

/** Relations can either be mandatory, optional or deprecated. */
enum validation_type { mandatory, optional, deprecated };
		
/**
 * Class for simplifying Xrsl validation. One object
 * of this class represents a valid attribute in the xrsl.
 */
class XrslValidationData {

	public:
		/** Constructor. */
		XrslValidationData(const std::string & attribute_name,
						   relation_type rel_type,
						   validation_type val_type,
		                   bool unique = true,
						   int list_length = 0);

		/** Name of attribute. */
		std::string attribute_name;

		/** Type the attribute must have. */
		relation_type rel_type;

		/** Must this attribute be unique? */
		bool unique;

		/** Length of each list in case attribute is a list of values. */
		int list_length;

		/** Must the Xrsl must contain this attribute to be valid? */
		validation_type val_type;
};


/**
 *  Class used to simplify manipulation of xRSL job descriptions.
 */
class Xrsl {

	public:

		/** Constructs a Xrsl object from a string respresentation. */
		Xrsl(const std::string &xrsl_string) throw(XrslError);

		/** Constructs empty Xrsl object. */
		Xrsl(xrsl_operator = operator_and);

		/** Construct Xrsl object from globus_rsl_t* . */
		Xrsl(globus_rsl_t*);

		/** Copy constructor. */
		Xrsl(const Xrsl &other_xrsl);

		/** Copy-assignment constructor. */
		Xrsl& operator=(const Xrsl &other_xrsl);

		/** Destructor. */
		~Xrsl();

		/** Print detailed information about each relation. */
		void Print() const;

		/** Converts the Xrsl object to std::string representation. */
		const std::string str() const throw(XrslError);

		/** If the Xrsl start with a +, split the Xrsl into multiple Xrsls. */
		std::list<Xrsl> SplitMulti();

		/** Splits an Xrsl containing or-operators into separate Xrsl's.
		 *  Example: &(executable=/bin/echo)(|(cluster=cl1)(cluster=cl2))
		 *  split into &(executable=/bin/echo)(cluster=cl1) and
		 *  &(executable=/bin/echo)(cluster=cl2). */
		std::list<Xrsl> SplitOrRelation() throw(XrslError);

		/** Adds a new relation. Throws exception if relation already
		 *  exists in the xrsl and force is not true.
		 */
		void AddRelation(const XrslRelation& relation,
		                 bool force = true) throw(XrslError);

		/** Adds simple relation specified by attribute, xrsl-operator and
		 *  value. Throws exception if relation already exists in the xrsl and
		 *  force is not true.
		 */
		void AddSimpleRelation(const std::string& attr,
		                       xrsl_operator op,
		                       const std::string& val,
		                       bool force = true) throw(XrslError);

		/** Adds a sub-Xrsl to the Xrsl. */
		void AddXrsl(Xrsl& axrsl) throw(XrslError);

		/** Gets the first XrslRelation corresponding to the attribute. */
		XrslRelation GetRelation(const std::string& attr) throw(XrslError);

		/** Get all XrslRelation's in the xrsl with attribute equal to
		 *  parameter attr.
		 */
		std::list<XrslRelation> GetAllRelations(const std::string& attr);

		/** Does the relation with this attribute exist? */
		bool IsRelation(const std::string&);

		/** Removes a relation. Throws an exception if the relation does not
		 *  exist in the xrsl. The relation may be of any type.
		 */
		void RemoveRelation(const std::string& attr) throw(XrslError);

		/** Ensures that the xrsl only contains valid attributes.
		 *  Throws exception if some attribute has invalid format,
		 *  or that some mandatory attribute is missing.
		 */
		void Validate(const std::list<XrslValidationData>& valid_attributes,
                      bool allow_unknown = false) throw (XrslError);

		/** Performs RSL alias substitution etc. */
		void Eval();

	private:
		/** Internal function that splits the xrsl into components after
		 *  or's in the xrsl. Helper method for SplitOrRelation().
		 */
		void SplitXrsl(std::list<Xrsl>* xrsls,
		               globus_list_t* alist,
		               bool insideanor = false) throw(XrslError);

		/** Finds and returns relation. It returns the number'th relation
		 *  with attribute equal to the passed attribute.
		 */
		void FindRelation(const std::string& attribute,
		                  globus_list_t** relation,
		                  unsigned int number = 1,
		                  globus_list_t* axrsl = NULL) const;

		/** This method validates the list length of the list after the
		 *  XrslValidationData given to the attribute.
		 */
		void ValidateListLength(globus_list_t*, const XrslValidationData&)
		throw(XrslError);

		/** This method validates the attribute after the XrslValidationData
		 *  given to the attribute.
		 */ 
		void ValidateAttribute(globus_rsl_t*, const XrslValidationData&)
		throw(XrslError);

		/** Finds first element in the Xrsl tree from the given axrsl
		 *  element.
		 */
		globus_list_t** FindHead(globus_rsl_t* axrsl = NULL) const;

		/** Internal representation is a globus_rsl_t. */
		globus_rsl_t* xrsl;
};


/** This method performs a standard Xrsl validation with the XrslValidation
 *  data from the standard Xrsl-variables.
 */
void PerformXrslValidation(Xrsl axrsl,
                           bool allow_unknown = false) throw(XrslError);


#endif // ARCLIB_XRSL
