#ifndef __ARC_CONFUSAPARSERUTILS_H__
#define __ARC_CONFUSAPARSERUTILS_H__

#include <string>
#include <inttypes.h>

#include <arc/message/MCC.h>
#include <arc/URL.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <arc/client/ClientInterface.h>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace Arc {

/**
 * Methods often needed in evaluation web pages from the Confusa WebSSO workflow
 */
class ConfusaParserUtils {
public:
	ConfusaParserUtils();
	virtual ~ConfusaParserUtils();
	/**
	 * urlencode the passed string
	 */
	static std::string urlencode(const std::string url);
	/**
	 * Urlencode the passed string with respect to the parameters.
	 * The difference to urlencode is that the parameters will keep their seperators, i.e. the ? and &
	 * separating parameters will be preserved.
	 */
	static std::string urlencode_params(const std::string url);
	/**
	 * Construct a lixml2 doc representation from the xml file
	 */
	static xmlDocPtr get_doc(const std::string xml_file);
	/**
	 * Destroy a libxml2 doc representation
	 */
	static void destroy_doc(xmlDocPtr doc);
	/**
	 * Get the part only within <body> and </body> in a HTML string
	 * For parsing, usually only this part is interesting.
	 */
	static std::string extract_body_information(const std::string html_string);
	/**
	 * Handle a single redirect step from the SAML2 WebSSO profile.
	 * Store the received cookie in *cookie and pass the given httpAttributes to the site during redirect.
	 */
	static std::string handle_redirect_step(Arc::MCCConfig cfg, const std::string remote_url, std::string *cookie = NULL, std::map<std::string, std::string> *httpAttributes = NULL);
	/**
	 * Evaluate the given xPathExpr on the document ptr.
	 * Return a string with the FIRST result if contentList is NULL.
	 * Return a string with the first result and all results, including the first one, in contentList if contentList is not null.
	 */
	static std::string evaluate_path(xmlDocPtr doc, const std::string xpathExpr, std::list<std::string> *contentList = NULL);
	/*
	 * get the url parameters as a key-value map from an url
	 */
	static std::map<std::string, std::string> get_url_params(std::string url);
	/*
	 * use this to add cookies to an existing cookie entry
	 */
	static void add_cookie(std::string *cookies, const std::string cookie);
private:
	static Logger logger;
};

}; // namespace Arc

#endif /* __ARC_CONFUSAPARSERUTILS_H__ */
