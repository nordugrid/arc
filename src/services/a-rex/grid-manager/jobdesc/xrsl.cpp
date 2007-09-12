#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <iostream>
#include <string>

//@ #include <arc/notify.h>
#include <arc/StringConv.h>
#include "xrsl.h"

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(A) dgettext("arclib", (A))
#else
#define _(A) (A)
#endif

//@ 
typedef enum {
	WARNING
} notify_level_t;
static std::ostream& notify(notify_level_t level) {
	return std::cerr;
}
//@ 


std::list<std::string> GetOneList(globus_list_t* list) throw(XrslError);

XrslRelation::XrslRelation(const std::string& attribute,
                           const xrsl_operator& op,
                           const std::string& str) {

	globus_rsl_value_t* rval =
	    globus_rsl_value_make_literal(strdup(str.c_str()));

	globus_list_t* rlist = globus_list_cons((void*)rval, NULL);
	globus_rsl_value_t* val = globus_rsl_value_make_sequence(rlist);

	relation = globus_rsl_make_relation(op, strdup(attribute.c_str()), val);
}


XrslRelation::XrslRelation(const std::string& attribute,
                           const xrsl_operator& op,
                           const std::list<std::string>& value) {

	globus_list_t* rlist = NULL;
	std::list<std::string>::const_reverse_iterator it;
	for (it = value.rbegin(); it != value.rend(); it++) {

		// Create nodes
		globus_rsl_value_t* value =
		    globus_rsl_value_make_literal(strdup(it->c_str()));

		globus_list_insert(&rlist, (void*)value);
	}

	// Create relation
	globus_rsl_value_t* val = globus_rsl_value_make_sequence(rlist);

	relation = globus_rsl_make_relation(op, strdup(attribute.c_str()), val);
}


XrslRelation::XrslRelation(const std::string& attribute,
                           const xrsl_operator& op,
                           const std::list<std::list<std::string> >& value) {

	globus_list_t* outer_list = NULL;
	for (std::list<std::list<std::string> >::const_iterator outer =
	     value.begin(); outer != value.end(); outer++) {

		// Create sequence for inner list:
		globus_list_t* inner_list = NULL;
		std::list<std::string>::const_reverse_iterator inner;
		for (inner = outer->rbegin(); inner != outer->rend(); inner++) {

			// Create nodes
			globus_rsl_value_t* inner_value =
			    globus_rsl_value_make_literal(strdup(inner->c_str()));

			inner_list = globus_list_cons((void *)inner_value, inner_list);
		}

		// Create node of outer list
		globus_rsl_value_t* outer_value =
		    globus_rsl_value_make_sequence(inner_list);

		outer_list = globus_list_cons((void *)outer_value, outer_list);
	}

	// Create relation
	globus_rsl_value_t* val = globus_rsl_value_make_sequence(outer_list);

	relation = globus_rsl_make_relation(op, strdup(attribute.c_str()), val);
}


XrslRelation::XrslRelation(globus_rsl_t* rel) {

	if (!globus_rsl_is_relation(rel))
		throw XrslError(_("Variable is not an RSL relation"));

	relation = globus_rsl_copy_recursive(rel);
}


XrslRelation::XrslRelation(const XrslRelation& other_relation) {
	relation = globus_rsl_copy_recursive(other_relation.relation);
}


XrslRelation::~XrslRelation() {
	if (relation) globus_rsl_free_recursive(relation);
}


std::string XrslRelation::str() {
	char* buffer = globus_rsl_unparse(relation);
	if (!buffer) return "";

	std::string str(buffer);
	free(buffer);
	return str;
}


std::string XrslRelation::GetAttribute() const {

	char* buffer = globus_rsl_relation_get_attribute(relation);
	if (!buffer) return "";

	std::string str(buffer);
	return str;
}


xrsl_operator XrslRelation::GetOperator() const {
	return (xrsl_operator)globus_rsl_relation_get_operator(relation);
}


globus_rsl_t* XrslRelation::GetRelation() const {
	return globus_rsl_copy_recursive(relation);
}


std::string XrslRelation::GetSingleValue() throw(XrslError) {

	globus_rsl_value_t* val = globus_rsl_relation_get_value_sequence(relation);

	globus_list_t* rlist = globus_rsl_value_sequence_get_value_list(val);
	if (globus_list_size(rlist) != 1)
		throw XrslError(_("Attribute is not single valued"));

	globus_rsl_value_t* litval = (globus_rsl_value_t*)globus_list_first(rlist);
	if (!globus_rsl_value_is_literal(litval))
		throw XrslError(_("Value is not a string literal"));

	return globus_rsl_value_literal_get_string(litval);
}


std::list<std::list<std::string > > XrslRelation::GetDoubleListValue()
throw(XrslError) {

	std::list<std::list<std::string> > result;

	char* buffer = globus_rsl_relation_get_attribute(relation);
	std::string attr = (buffer ? buffer : "");

	globus_rsl_value_t* val = globus_rsl_relation_get_value_sequence(relation);

	globus_list_t* list = globus_rsl_value_sequence_get_value_list(val);

	// Iterate through outer list, save all lists in result
	while (!globus_list_empty(list)) {
		globus_rsl_value_t* value =
		    (globus_rsl_value_t*)globus_list_first(list);

		globus_list_t* inner_list =
		    globus_rsl_value_sequence_get_value_list(value);

		std::list<std::string> result_list;
		try {
			result_list = GetOneList(inner_list);
		} catch (XrslError e) {
			throw XrslError(_("Syntax error in attribute") + (": " + attr));
		}

		result.push_back(result_list);
		list = globus_list_rest(list);
	}

	return result;
}


std::list<std::string > XrslRelation::GetListValue() throw(XrslError) {

	std::list<std::string> result;

	char* buffer = globus_rsl_relation_get_attribute(relation);
	std::string attr = (buffer ? buffer : "");

	globus_rsl_value_t* val = globus_rsl_relation_get_value_sequence(relation);
	globus_list_t* rlist = globus_rsl_value_sequence_get_value_list(val);

	// Iterate through list, save all lists in result
	while (!globus_list_empty(rlist)) {
		globus_rsl_value_t* listvalue =
		    (globus_rsl_value_t*)globus_list_first(rlist);

		if (!globus_rsl_value_is_literal(listvalue))
			throw XrslError(_("Syntax error in list"));

		std::string resvalue(globus_rsl_value_literal_get_string(listvalue));
		result.push_back(resvalue);

		rlist = globus_list_rest(rlist);
	}

	return result;
}


Xrsl::Xrsl(const std::string& xrsl_string) throw(XrslError) {

	xrsl = globus_rsl_parse((char*)xrsl_string.c_str());
	if (!xrsl)
		throw XrslError(
			_("Xrsl string could not be parsed") + (": " + xrsl_string));
}


// Create an empty Xrsl object with an xrsl_operator
Xrsl::Xrsl(xrsl_operator op) {
	xrsl = globus_rsl_make_boolean(op, NULL);
}


Xrsl::Xrsl(globus_rsl_t* rsl) {
	xrsl = globus_rsl_copy_recursive(rsl);
}


Xrsl::Xrsl(const Xrsl& other_xrsl) {
	xrsl = globus_rsl_copy_recursive(other_xrsl.xrsl);
}


Xrsl& Xrsl::operator=(const Xrsl& other_xrsl) {
	if (xrsl) globus_rsl_free_recursive(xrsl);
	xrsl = globus_rsl_copy_recursive(other_xrsl.xrsl);
	return *this;
}


Xrsl::~Xrsl() {
	if (xrsl) globus_rsl_free_recursive(xrsl);
}


void Xrsl::Print() const {
	if (xrsl) globus_rsl_print_recursive(xrsl);
}


std::list<Xrsl> Xrsl::SplitMulti() {

	std::list<Xrsl> multixrsl;

	if (globus_rsl_is_boolean_multi(xrsl)) {
		globus_list_t* rlist = globus_rsl_boolean_get_operand_list(xrsl);
		while (!globus_list_empty(rlist)) {
			multixrsl.push_back(Xrsl((globus_rsl_t*)globus_list_first(rlist)));

			rlist = globus_list_rest(rlist);
		}
	} else {
		multixrsl.push_back(Xrsl(xrsl));
	}

	return multixrsl;
}


std::list<Xrsl> Xrsl::SplitOrRelation() throw(XrslError) {

	std::list<Xrsl> multixrsl;
	multixrsl.push_back(Xrsl());

	globus_list_t* alist = *FindHead();
	SplitXrsl(&multixrsl, alist);

	return multixrsl;
}


void Xrsl::SplitXrsl(std::list<Xrsl>* xrsls,
                     globus_list_t* alist,
                     bool insideanor) throw(XrslError) {

	std::list<Xrsl> somexrsls;
	if (insideanor)
		somexrsls = *xrsls;

	std::list<Xrsl>::iterator xrslit = xrsls->begin();

	while (!globus_list_empty(alist)) {

		globus_rsl_t* arel = (globus_rsl_t*)globus_list_first(alist);
		if (globus_rsl_is_boolean_and(arel)) {
			SplitXrsl(xrsls, globus_rsl_boolean_get_operand_list(arel), false);
		}
		else if (globus_rsl_is_boolean_or(arel)) {
			SplitXrsl(xrsls, globus_rsl_boolean_get_operand_list(arel), true);
		}
		else if (globus_rsl_is_relation(arel)) {
			std::list<Xrsl>::iterator it;
			for (it = xrslit; it != xrsls->end(); it++)
				it->AddRelation(arel);
		}

		alist = globus_list_rest(alist);

		if (insideanor && !globus_list_empty(alist)) {
			std::list<Xrsl>::iterator it;
			for (it = somexrsls.begin(); it != somexrsls.end(); it++) {
				xrsls->push_back(*it);
				xrslit++;
			}
		}
	}
}


const std::string Xrsl::str() const throw(XrslError) {

	char* buffer = globus_rsl_unparse(xrsl);
	if (!buffer) throw XrslError(_("Illegal xrsl"));

	const std::string xrslstr = buffer;
	free(buffer);
	return xrslstr;
}


void Xrsl::AddXrsl(Xrsl& axrsl) throw(XrslError) {
	globus_list_t** head = FindHead();

	globus_rsl_t* listxrsl = globus_rsl_copy_recursive(axrsl.xrsl);
	globus_list_insert(head, (globus_list_t*)listxrsl);
}


void Xrsl::AddRelation(const XrslRelation& relation,
                       bool force) throw(XrslError) {

	globus_list_t* oldrelation = NULL;

	// Ensure that relation does not exist before inserting it
	if (!force) {
		FindRelation(relation.GetAttribute(), &oldrelation);
		if (oldrelation)
			throw XrslError(_("Attribute already exists") +
			                (": " + relation.GetAttribute()));
	}

	globus_list_t** head = FindHead();
	globus_list_insert(head, (void*)relation.GetRelation());
}


void Xrsl::AddSimpleRelation(const std::string& attr,
                             xrsl_operator op,
                             const std::string& val,
                             bool force) throw(XrslError) {

	XrslRelation relation(attr, op, val);
	AddRelation(relation);
}


/** Finds first relation with attribute equal to attr. */
XrslRelation Xrsl::GetRelation(const std::string& attr) throw(XrslError) {

	// Assert that relation exists
	globus_list_t* relation = NULL;
	FindRelation(attr, &relation);
	if (!relation)
		throw XrslError(_("Attribute not found") + (": " + attr));

	XrslRelation newrel((globus_rsl_t*)globus_list_first(relation));
	return newrel;
}


/** Returns all relations for a specific attribute. */
std::list<XrslRelation> Xrsl::GetAllRelations(const std::string& attr) {

	std::list<XrslRelation> allrels;

	unsigned int i = 1;
	while (true) {
		globus_list_t* relation = NULL;
		FindRelation(attr, &relation, i);
		if (!relation) break;

		XrslRelation newrel((globus_rsl_t*)globus_list_first(relation));
		allrels.push_back(newrel);
		i++;
	}

	return allrels;
}


// Exception is thrown if relation can not be found or removed.
void Xrsl::RemoveRelation(const std::string& attr) throw(XrslError) {

	globus_list_t* rel = NULL;

	FindRelation(attr, &rel);
	if (!rel)
		throw XrslError(_("Attribute not found") + (": " + attr));

	globus_list_t** head = FindHead();

	if (globus_rsl_free_recursive((globus_rsl_t*)globus_list_first(rel)))
		throw XrslError(_("Cannot remove relation") + (": " + attr));
	if (globus_list_remove(head, rel)==NULL)
		throw XrslError(_("Cannot remove relation") + (": " + attr));
}


bool Xrsl::IsRelation(const std::string& attr) {

	globus_list_t* rel = NULL;

	FindRelation(attr, &rel);

	return rel;
}


// Performs alias substitution etc.
void Xrsl::Eval() {

	globus_symboltable_t table;

	globus_symboltable_init(&table,
	                        globus_hashtable_string_hash,
	                        globus_hashtable_string_keyeq);
	globus_rsl_eval(xrsl, &table);
	globus_symboltable_destroy(&table);
}


// Finds a relation, returns nonzero upon error.
// If relation is found, pointer to it is stored in second argument
void Xrsl::FindRelation(const std::string& attr,
                        globus_list_t** relation,
                        unsigned int number,
                        globus_list_t* alist) const {
                        
	if (!alist) {
	    alist = *FindHead();
		*relation = NULL;
	}

	if (*relation) return;

	while (!globus_list_empty(alist)) {

		globus_rsl_t* arel = (globus_rsl_t*)globus_list_first(alist);
		if (globus_rsl_is_boolean(arel)) {
			FindRelation(attr,
			             relation,
			             number,
			             globus_rsl_boolean_get_operand_list(arel));
		}
		else if (globus_rsl_is_relation(arel)) {
			char* relattr = globus_rsl_relation_get_attribute(arel);
			if (strcasecmp(relattr, attr.c_str()) == 0) {
				number--;
				if (number == 0) {
					*relation = alist;
					break;
				}
			}
		}

		alist = globus_list_rest(alist);
	}
}


// Returns first element in list (tree)
globus_list_t** Xrsl::FindHead(globus_rsl_t* axrsl) const {

	if (!axrsl) axrsl = xrsl;
	if (!globus_rsl_is_boolean(axrsl)) return NULL;

	return globus_rsl_boolean_get_operand_list_ref(axrsl);
}


// Ensure that no invalid attributes exist in xrsl
void Xrsl::Validate(const std::list<XrslValidationData>& valid_attributes,
                    bool allow_unknown) throw (XrslError) {

	globus_list_t* xrsl_relation = NULL;

	for (std::list<XrslValidationData>::const_iterator valid_ones =
	    valid_attributes.begin(); valid_ones != valid_attributes.end();
		valid_ones++) {

		std::string attr = valid_ones->attribute_name;

		// Ensure that the xrsl contains all mandatory attributes
		if (valid_ones->val_type == mandatory) {
			xrsl_relation = NULL;
			FindRelation(attr, &xrsl_relation);

			if (!xrsl_relation) {
				throw XrslError(
					_("Xrsl does not contain the mandatory relation") +
					(": " + valid_ones->attribute_name));
			}
		}

		// Warn against deprecated attributes
		if (valid_ones->val_type == deprecated) {
			xrsl_relation = NULL;
			FindRelation(attr, &xrsl_relation);

			if (xrsl_relation) {
				notify(WARNING)
					<< _("The xrsl contains the deprecated attribute")
					<< ": " << valid_ones->attribute_name + " - "
					<< _("It will be ignored") << std::endl;
			}
		}

		/* Ensure that all attributes that are supposed to be unique
		   really is it. I.e. two executable attributes are not allowed. */
		if (valid_ones->unique) {
			std::list<XrslRelation> all = GetAllRelations(attr);
			if (all.size() > 1) {
				throw XrslError(_("The xrsl contains more than one relation "
				                  "with attribute") + (": " + attr) + " - " +
				                _("This attribute is supposed to be unique"));
			}
		}
	}

	/* Next, ensure that all relations have a valid attribute name,
	   a valid type, and in the list case, lists are of valid length. */
	globus_rsl_t* relation;
	// Iterate through all relations
	if (globus_rsl_is_boolean(xrsl)) {
		globus_list_t* list = globus_rsl_boolean_get_operand_list(xrsl);

		while (!globus_list_empty(list)) {
			relation = (globus_rsl_t*)globus_list_first(list);

			if (globus_rsl_is_relation(relation)) {

				std::string attribute_name =
					globus_rsl_relation_get_attribute(relation);

				// Find the ValidationData that corresponds to this relation
				// if no data can be found, the relation is illegal
				bool found = false;
				for (std::list<XrslValidationData>::const_iterator
				     valid_ones = valid_attributes.begin();
				     valid_ones != valid_attributes.end(); valid_ones++) {

					// Corresponding Validation data found -> name is valid
					if (strcasecmp(valid_ones->attribute_name.c_str(),
					    attribute_name.c_str()) == 0) {

						// This throws exception if something is
						// wrong with relation
						ValidateAttribute(relation, *valid_ones);
						found = true;
					}
				}
				if (!found) {
					if(!allow_unknown) {
						throw XrslError(
						  _("Not a valid attribute") + (": " + attribute_name));
					}
					notify(WARNING)
						<< _("The xrsl contains unknown attribute")
						<< ": " << attribute_name << std::endl;
				}
			}
			else {
				throw XrslError(
					_("Xrsl contains something that is not a relation"));
			}

			list = globus_list_rest (list);
		}
	}
	else {
		throw XrslError(_("Malformed xrsl expression"));
	}
}


// Ensure that all lists in list has valid length
//
void Xrsl::ValidateListLength(globus_list_t *list,
                              const XrslValidationData& validation_data)
throw(XrslError) {

	// Any length allowed
	if (validation_data.list_length == -1)  // -1 = ANY_LENGTH
		return;

	// Test size of all sub lists
	while (!globus_list_empty(list)) {

		// Get inner list
		globus_rsl_value_t* inner_value =
		    (globus_rsl_value_t*)globus_list_first(list);

		// Ensure that value indead is a list.
		if (!globus_rsl_value_is_sequence(inner_value))
			throw XrslError(_("Attribute must be of type list") +
			                (": " + validation_data.attribute_name));

		globus_list_t* inner_list =
		    globus_rsl_value_sequence_get_value_list(inner_value);

		if (globus_list_size(inner_list)!=validation_data.list_length)
			throw XrslError(
				_("Attribute must consist only of lists of length") +
				(" " + Arc::tostring(validation_data.list_length)) +
				": " + validation_data.attribute_name);

		list = globus_list_rest(list);
	}
}


// Validates one attribute
// Throws XrslError if attribute is invalid
void Xrsl::ValidateAttribute(globus_rsl_t* attribute,
const XrslValidationData& validation_data) throw(XrslError) {

	// Remove all common layers of string and lists relations
	globus_rsl_value_t* sequence =
	    globus_rsl_relation_get_value_sequence(attribute);

	globus_list_t * value_list =
	    globus_rsl_value_sequence_get_value_list(sequence);

	globus_rsl_value_t * value =
	    (globus_rsl_value_t*)globus_list_first(value_list);

	// Ensure that type is correct
	if (validation_data.rel_type == RELATIONTYPE_STRING) {
		if (!globus_rsl_value_is_literal(value))
			throw XrslError(_("Attribute must be of type string") +
			                (": " + validation_data.attribute_name));
		else
			return; // type is string, no further tests are needed
	}
	if (validation_data.rel_type == RELATIONTYPE_LIST) {
		if (!globus_rsl_value_is_sequence(value))
			throw XrslError(_("Attribute must be of type list") +
			                (": " + validation_data.attribute_name));
	}

	// Get list
	globus_rsl_value_t* list_sequence =
	globus_rsl_relation_get_value_sequence(attribute);

	globus_list_t* list =
	globus_rsl_value_sequence_get_value_list(list_sequence);

	ValidateListLength(list, validation_data);
}


XrslValidationData::XrslValidationData(const std::string & attr_name,
                                       relation_type rel,
                                       validation_type val,
                                       bool uniqueness,
                                       int list_len) {
	attribute_name = attr_name;
	rel_type = rel;
	val_type = val;
	unique = uniqueness;
	list_length = list_len;
}


// Helper function, extracts strings from sequence
// and stores them into a list.
std::list<std::string> GetOneList(globus_list_t* list) throw(XrslError) {

	std::list<std::string> result;

	while (!globus_list_empty(list)) {

		globus_rsl_value_t* value =
		    (globus_rsl_value_t*)globus_list_first(list);

		// All items in the list should be literal strings
		if (globus_rsl_value_is_literal(value)) {
			result.push_back(globus_rsl_value_literal_get_string(value));
		}
		else {
			throw XrslError(_("Syntax error in list"));
		}

		list = globus_list_rest(list);
	}

	return result;
}


void PerformXrslValidation(Xrsl axrsl,
                           bool allow_unknown) throw(XrslError) {

	// Lets construct XrslValidationData
	std::list<XrslValidationData> valid_attributes;

	// The valid attributes
	XrslValidationData executable("executable",
	                              RELATIONTYPE_STRING,
	                              mandatory);
	valid_attributes.push_back(executable);

	XrslValidationData arguments("arguments", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(arguments);

	XrslValidationData executables("executables",
	                               RELATIONTYPE_STRING,
	                               optional);
	valid_attributes.push_back(executables);

	XrslValidationData jobname("jobname", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(jobname);

	XrslValidationData sstdin("stdin", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(sstdin);

	XrslValidationData sstdout("stdout", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(sstdout);

	XrslValidationData sstderr("stderr", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(sstderr);

	XrslValidationData gmlog("gmlog", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(gmlog);

	XrslValidationData join("join", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(join);

	XrslValidationData inputfiles("inputfiles",
	                              RELATIONTYPE_LIST,
	                              optional, true, 2);
	valid_attributes.push_back(inputfiles);

	XrslValidationData outputfiles("outputfiles",
	                               RELATIONTYPE_LIST,
	                               optional, true, 2);
	valid_attributes.push_back(outputfiles);

	XrslValidationData count("count", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(count);

	XrslValidationData notify("notify", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(notify);

	XrslValidationData cluster("cluster", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(cluster);

	XrslValidationData queue("queue", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(queue);

	XrslValidationData starttime("starttime", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(starttime);

	XrslValidationData cputime("cputime", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(cputime);

	XrslValidationData walltime("walltime", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(walltime);

	XrslValidationData gridtime("gridtime", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(gridtime);

	XrslValidationData lifetime("lifetime", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(lifetime);

	XrslValidationData memory("memory", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(memory);

	XrslValidationData disk("disk", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(disk);

	XrslValidationData opsys("opsys", RELATIONTYPE_STRING, optional, false);
	valid_attributes.push_back(opsys);

	XrslValidationData rerun("rerun", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(rerun);

	XrslValidationData dryrun("dryrun", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(dryrun);

	XrslValidationData architecture("architecture",
	                                RELATIONTYPE_STRING,
	                                optional,
	                                false);
	valid_attributes.push_back(architecture);

	XrslValidationData replicacollection("replicacollection",
	                                     RELATIONTYPE_STRING,
	                                     optional,
	                                     false);
	valid_attributes.push_back(replicacollection);

	XrslValidationData rsl_substitution("rsl_substitution",
	                                    RELATIONTYPE_LIST,
	                                    optional,
	                                    false, 2);
	valid_attributes.push_back(rsl_substitution);

	XrslValidationData middleware("middleware",
	                              RELATIONTYPE_STRING,
	                              optional,
	                              false);
	valid_attributes.push_back(middleware);

	XrslValidationData ftpthreads("ftpthreads",
	                              RELATIONTYPE_STRING,
	                              optional);
	valid_attributes.push_back(ftpthreads);

	XrslValidationData jobtype("jobtype", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(jobtype);

	XrslValidationData cache("cache", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(cache);

	XrslValidationData nodeaccess("nodeaccess",
	                              RELATIONTYPE_STRING,
	                              optional);
	valid_attributes.push_back(nodeaccess);

	XrslValidationData jobreport("jobreport", RELATIONTYPE_STRING, optional);
	valid_attributes.push_back(jobreport);

	XrslValidationData credserver("credentialserver",
	                              RELATIONTYPE_STRING,
	                              optional);
	valid_attributes.push_back(credserver);

	XrslValidationData benchmarks("benchmarks",
	                              RELATIONTYPE_LIST,
	                              optional, true, 3);
	valid_attributes.push_back(benchmarks);

	XrslValidationData environment("environment",
	                               RELATIONTYPE_LIST,
	                               optional,
	                               false, 2);
	valid_attributes.push_back(environment);

	XrslValidationData runtimeenvironment("runtimeenvironment",
	                                      RELATIONTYPE_STRING,
	                                      optional,
	                                      false);
	valid_attributes.push_back(runtimeenvironment);

	// The deprecated ones
	XrslValidationData resourcemanagercontact("resourcemanagercontact",
	                                          RELATIONTYPE_STRING,
	                                          deprecated);
	valid_attributes.push_back(resourcemanagercontact);

	XrslValidationData directory("directory",
	                             RELATIONTYPE_STRING,
	                             deprecated);
	valid_attributes.push_back(directory);

	XrslValidationData maxwalltime("maxwalltime",
	                               RELATIONTYPE_STRING,
	                               deprecated);
	valid_attributes.push_back(maxwalltime);

	XrslValidationData maxcputime("maxcputime",
	                              RELATIONTYPE_STRING,
	                              deprecated);
	valid_attributes.push_back(maxcputime);

	XrslValidationData maxtime("maxtime", RELATIONTYPE_STRING, deprecated);
	valid_attributes.push_back(maxtime);

	XrslValidationData maxmemory("maxmemory",
	                             RELATIONTYPE_STRING,
	                             deprecated);
	valid_attributes.push_back(maxmemory);

	XrslValidationData minmemory("minmemory",
	                             RELATIONTYPE_STRING,
	                             deprecated);
	valid_attributes.push_back(minmemory);

	XrslValidationData maxdisk("maxdisk", RELATIONTYPE_STRING, deprecated);
	valid_attributes.push_back(maxdisk);

	XrslValidationData stdlog("stdlog", RELATIONTYPE_STRING, deprecated);
	valid_attributes.push_back(stdlog);

	XrslValidationData grammyjob("grammyjob",
	                             RELATIONTYPE_STRING,
	                             deprecated);
	valid_attributes.push_back(grammyjob);

	XrslValidationData project("project", RELATIONTYPE_STRING, deprecated);
	valid_attributes.push_back(project);

	XrslValidationData hostcount("hostcount",
	                             RELATIONTYPE_STRING,
	                             deprecated);
	valid_attributes.push_back(hostcount);

	XrslValidationData parallelenvironment("parallelenvironment",
	                                       RELATIONTYPE_STRING,
	                                       deprecated);
	valid_attributes.push_back(parallelenvironment);

	XrslValidationData label("label", RELATIONTYPE_STRING, deprecated);
	valid_attributes.push_back(label);

	XrslValidationData subjobcommstype("subjobcommstype",
	                                   RELATIONTYPE_STRING,
	                                   deprecated);
	valid_attributes.push_back(subjobcommstype);

	XrslValidationData subjobstarttype("subjobstarttype",
	                                   RELATIONTYPE_STRING,
	                                   deprecated);
	valid_attributes.push_back(subjobstarttype);

	// Lets split the Xrsl into a list of Xrsls
	std::list<Xrsl> allxrsls;

	allxrsls = axrsl.SplitMulti();
	std::list<Xrsl>::iterator it;
	for (it = allxrsls.begin(); it != allxrsls.end(); it++) {
		std::list<Xrsl> xrsllist = it->SplitOrRelation();

		std::list<Xrsl>::iterator it2;
		for (it2 = xrsllist.begin(); it2 != xrsllist.end(); it2++) {
			try {
				it2->Validate(valid_attributes, allow_unknown);
			} catch(XrslError e) {
				throw XrslError(_("The xrsl is invalid") + std::string(": ") +
				                e.what());
			}
		}
	}
}
