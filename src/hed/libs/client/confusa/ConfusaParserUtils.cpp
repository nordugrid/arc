/*
 * ParserUtils.cpp
 *
 *  Created on: Apr 8, 2009
 *      Author: tzangerl
 */

#include "ConfusaParserUtils.h"

namespace Arc {

Logger ConfusaParserUtils::logger(Logger::getRootLogger(), "ConfusaUtils");

ConfusaParserUtils::ConfusaParserUtils() {
}

ConfusaParserUtils::~ConfusaParserUtils() {
}

std::map<std::string, std::string> ConfusaParserUtils::get_url_params(std::string url) {
	std::map<std::string, std::string> res_map;

	std::string::size_type first_param_pos = url.find("?");

	// if no query parameter sign is found in the string, assume that it only consists of parameters
	if (first_param_pos == std::string::npos) {
		first_param_pos = 0;
	} else {
		first_param_pos++;  // move pointer to actual position of the first parameter string
	}

	while (first_param_pos <= url.size()) {
		std::string::size_type next_param_pos = url.find("&", first_param_pos)+1;

		if (next_param_pos <= first_param_pos) {
			next_param_pos = url.size()+1;
		}

		std::string::size_type key_val_delim_pos = url.find("=", first_param_pos);

		if (key_val_delim_pos == std::string::npos) {
			break;  // no parameter whatsoever in string, or corrupted last parameter
		}

		std::string key = url.substr(first_param_pos, (key_val_delim_pos-first_param_pos));
		std::string value = url.substr(key_val_delim_pos+1, (next_param_pos-key_val_delim_pos-2));

		first_param_pos = next_param_pos;

		res_map[key] = value;
	}

	return res_map;

}

// sometimes the parameters of an URL are not urlencoded
std::string ConfusaParserUtils::urlencode_params(const std::string url) {
	std::string uri_part = "";
	std::string replaced_request = "";
	std::string::size_type first_param_pos = url.find("?");

	// if no query parameter sign is found in the string, assume that it only consists of parameters
	if (first_param_pos == std::string::npos) {
		first_param_pos = 0;
	} else {
		uri_part = url.substr(0, first_param_pos);
		replaced_request = "?";
		first_param_pos++;
	}

	while (first_param_pos <= url.size()) {
		std::string::size_type next_param_pos = url.find("&", first_param_pos)+1;

		if (next_param_pos <= first_param_pos) {
			next_param_pos = url.size()+1;
		}

		std::string::size_type key_val_delim_pos = url.find("=", first_param_pos);

		if (key_val_delim_pos == std::string::npos) {
			break;  // no parameter whatsoever in string, or corrupted last parameter
		}

		std::string key = url.substr(first_param_pos, (key_val_delim_pos-first_param_pos));
		std::string value = url.substr(key_val_delim_pos+1, (next_param_pos-key_val_delim_pos-2));

		replaced_request += key + "=" + ConfusaParserUtils::urlencode(value) + "&";

		first_param_pos = next_param_pos;


	}

	// The last "&" is too much, remove it
	replaced_request = replaced_request.substr(0, replaced_request.size()-1);
	return (uri_part + replaced_request);
}

// TODO: make this function complete. For the beginning, include only needed characters
std::string ConfusaParserUtils::urlencode(const std::string url) {
	  std::string result = url;

	  std::map<std::string, std::string> lut;
	  lut.insert(std::make_pair(":", "%3A"));
	  lut.insert(std::make_pair("/", "%2F"));
	  lut.insert(std::make_pair("?", "%3F"));
	  lut.insert(std::make_pair("+", "%2B"));
	  lut.insert(std::make_pair(" ", "%20"));
	  lut.insert(std::make_pair("!", "%21"));
	  lut.insert(std::make_pair("#", "%23"));
	  lut.insert(std::make_pair("$", "%24"));
	  lut.insert(std::make_pair("&", "%26"));
	  lut.insert(std::make_pair("'", "%27"));
	  lut.insert(std::make_pair("(", "%28"));
	  lut.insert(std::make_pair(")", "%29"));
	  lut.insert(std::make_pair("*", "%2A"));
	  lut.insert(std::make_pair(">", "%3E"));
	  lut.insert(std::make_pair("<", "%3C"));

	  std::map<std::string, std::string>::iterator it;

	  for (it=lut.begin(); it != lut.end(); it++) {
		  std::string::size_type token_pos = 0;

		  while ((token_pos = result.find((*it).first,token_pos)) != std::string::npos) {
			  result.erase(token_pos, 1);
			  result.insert(token_pos, (*it).second);
		  }
	  }

	  return result;
}

/**
* Take the xmlDocPtr and apply the xpathExpr xpathExpr on it
* store all the results in contentList
* return the first found element in a string
*/
std::string ConfusaParserUtils::evaluate_path(xmlDocPtr doc, const std::string xpathExpr, std::list<std::string> *contentList) {

	  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
	  xmlChar *searchTerm = (xmlChar *) xpathExpr.c_str();
	  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(searchTerm, xpathCtx);
	  xmlNodeSetPtr nodeSet = xpathObj->nodesetval;

	  if (nodeSet == NULL) {
		  return "";
	  }

	  if (nodeSet->nodeNr == 0) {
		  return "";
	  }

	  std::string first_content;
	 	  std::string content;

	  if ((nodeSet->nodeTab[0]->type == XML_ELEMENT_NODE) || (nodeSet->nodeTab[0]->type == XML_ATTRIBUTE_NODE)) {
	  			  first_content = ((char *) nodeSet->nodeTab[0]->children->content);
	  			  Arc::Logger::getRootLogger().msg(Arc::VERBOSE, "Trying to get content %s from XML element, size %d", first_content, nodeSet->nodeNr);
	  		  } else {
	  			  first_content = "";
	  }

	  // if there is a contentList parameter, enter the first content and fill up the rest of the parameters
	  if (contentList) {
		  std::list<std::string>::iterator it = contentList->begin();
		  contentList->insert(it,first_content);

		  for (int i = 1; i < nodeSet->nodeNr; i++) {
			  if ((nodeSet->nodeTab[i]->type == XML_ELEMENT_NODE) || (nodeSet->nodeTab[i]->type == XML_ATTRIBUTE_NODE)) {
				  content = ((char *) nodeSet->nodeTab[i]->children->content);
				  Arc::Logger::getRootLogger().msg(Arc::VERBOSE, "Trying to get content %s from XML element, size %d", content, nodeSet->nodeNr);
			  } else {
				  content = "";
			  }
			  contentList->insert(++it,content);

		  }
	  }

	  return first_content;
}


xmlDocPtr ConfusaParserUtils::get_doc(const std::string xml_file) {
	  xmlDocPtr doc = NULL;

	  doc = xmlReadMemory(xml_file.c_str(), xml_file.size(), "noname.xml", NULL, 0);

	  if (doc == NULL) {
		  Arc::Logger::getRootLogger().msg(Arc::ERROR, "Failed to parse XML file!");
	  }

	  return doc;
}

void ConfusaParserUtils::destroy_doc(xmlDocPtr doc) {
	  xmlFreeDoc(doc);
}

// extract the body from an HTML string
std::string ConfusaParserUtils::extract_body_information(const std::string html_string) {
	std::string lower_string = "";

	  // transfer HTML to lower case
	  const int length = html_string.length();
	  for(int i=0; i < length; i++) {
	      lower_string += std::tolower(html_string[i]);
	  }

	  std::string::size_type first_body_occurence = lower_string.find("<body");
	  std::string::size_type last_body_occurence = lower_string.find("</body>") + 7;

	  if (first_body_occurence == std::string::npos || last_body_occurence == std::string::npos) {
		  logger.msg(ERROR, "extract_body_information(): Body elements not found in passed string");
		  return "";
	  }

	  std::string body_string = html_string.substr(first_body_occurence, (last_body_occurence - first_body_occurence));

	 //  TODO: this is a hack, replace later, check for namespace assignment
	  std::string::size_type copy_pos = body_string.find("&copy;");
	  if (copy_pos != std::string::npos && (copy_pos + 6) <= body_string.size()) {
		  body_string.erase(copy_pos, 6);
	  }

	  copy_pos = body_string.find("&raquo;");

	  if (copy_pos != std::string::npos && (copy_pos + 7) <= body_string.size()) {
		  body_string.erase(copy_pos, 7);
	  }

	  return body_string;
}

// perform one redirect step, i.e. call the url, read the response and hand back the new url to which the redirect happens
// there are amazingly lots of redirects in web-based SSO slcs
std::string ConfusaParserUtils::handle_redirect_step(Arc::MCCConfig cfg, const std::string remote_url, std::string *cookies, std::multimap<std::string, std::string> *httpAttributes) {
	  Arc::URL url(urlencode_params(remote_url));
	  Arc::ClientHTTP redirect_client(cfg, url);
	  Arc::PayloadRaw redirect_request;
	  Arc::PayloadRawInterface *redirect_response = NULL;
	  Arc::HTTPClientInfo redirect_info;
	  redirect_client.RelativeURI(true);

	  if (httpAttributes != NULL) {
		  redirect_client.process("GET", *httpAttributes, &redirect_request, &redirect_info, &redirect_response);
	  } else {
		  redirect_client.process("GET", &redirect_request, &redirect_info, &redirect_response);
	  }

	  std::string redirect_string = redirect_info.location;
//	  if(!redirect_info.cookies.empty() && (cookie != NULL))  {
//		  *cookie = *(redirect_info.cookies.begin());
//	  }
	  add_cookie(cookies, redirect_info.cookies);

	  if (redirect_response) {
		  delete redirect_response;
	  }

	  return redirect_string;
}

void ConfusaParserUtils::add_cookie(std::string *cookies, std::list<std::string> cookies_list) {
	// drop path, secure, etc.

	std::list<std::string>::iterator it;
	std::string current = "";
	std::string::size_type semic_pos;
	std::string current_cookie = "";

	for (it = cookies_list.begin(); it != cookies_list.end(); it++) {
		current = (*it);
		semic_pos = current.find(";");
		current_cookie = current.substr(0, semic_pos);
		// TODO improvable
		if (cookies->empty()) {
			(*cookies) = current_cookie;
		} else {
			(*cookies) = (*cookies) + "; " + current_cookie;
		}
	}



}


} // namespace Arc
