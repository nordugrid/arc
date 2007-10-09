#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>

#include "configcore.h"
#include "xmlconfig.h"

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

namespace ARex {

static void XmlErrorHandler(void* ctx, const char* msg) {
	return;
}


void XMLConfig::FillTree(xmlNode* node, Config& config) {

	bool has_element = false;

	for (xmlNode* cur_node = node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE)
			has_element = true;
		if (cur_node->children)
			FillTree(cur_node->children, config);
	}

	if (has_element)
		return;

	std::string key;
	std::string id;
	std::string attr;
	std::map<std::string,std::string> suboptions;

	static xmlNode* reg = NULL;
	bool newreg = false;

	for (xmlNode* cur_node = node; cur_node->parent->type != XML_DOCUMENT_NODE;
	     cur_node = cur_node->parent) {
		if (cur_node->type == XML_ELEMENT_NODE) {

			// hack for registrations that are not distinguished by id
			if (strcmp((const char*) cur_node->name, "registration") == 0) {
				if (reg != cur_node) {
					newreg = true;
					reg = cur_node;
				}
			}

			for (xmlAttr *attribute = cur_node->properties; attribute;
			     attribute = attribute->next) {
				if (strcmp((const char*) attribute->name, "id") == 0)
					id = (const char*) attribute->children->content;
				else
					suboptions[(const char*) attribute->name] =
						(const char*) attribute->children->content;
			}
			if (attr.empty())
				attr = (const char*) cur_node->name;
			else if (key.empty())
				key = (const char*) cur_node->name;
			else
				key = (const char*) cur_node->name + ('/' + key);
		}
	}
	Option opt(attr, (const char*) node->content, suboptions);

	try {
		if (newreg)
			throw ConfigError("");
		const ConfGrp& grp = config.FindConfGrp(key,id);
		const_cast<ConfGrp&>(grp).AddOption(opt);
	}
	catch (ConfigError) {
		ConfGrp grp(key,id);
		grp.AddOption(opt);
		config.AddConfGrp(grp);
	}
}


Config XMLConfig::Read(std::istream& is) {

	Config config;

	// Create a parser context
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	if (ctxt == NULL)
		throw ConfigError("Failed to create parser context");

	// Read stream content into buffer
	std::streamsize size = 4096;
	std::streamsize length = 0;
	char* buffer = (char*) malloc(sizeof(char) * (size + 1));
	if (!buffer)
		throw ConfigError("Failed to allocate memory for parser context");
	while(length < size) {
		is.read(buffer + length, size - length);
		length += is.gcount();
		if (length == size)
			size += 4096;
		else
			size = length;
		char* buffer_ = (char*) realloc(buffer, sizeof(char) * (size + 1));
		if (!buffer_) {
			free(buffer);
			throw ConfigError("Failed to allocate memory for parser context");
		}
		buffer = buffer_;
	}
	buffer[length] = '\0';

	// Set Error-handler
	xmlSetGenericErrorFunc(NULL, (xmlGenericErrorFunc)XmlErrorHandler);

	// Parse and validate buffer and fill the tree
	xmlDocPtr doc;
	doc = xmlParseMemory(buffer, length);
	free(buffer);

	// Reset Error-handler
	xmlSetGenericErrorFunc(NULL, NULL);

	// Check if parsing suceeded
	if (doc == NULL) {
		xmlFreeParserCtxt(ctxt);
		throw ConfigError("Failed xml parsing");
	}
	// Check if validation suceeded
	else if (ctxt->valid == 0) {
		xmlFreeParserCtxt(ctxt);
		xmlFreeDoc(doc);
		throw ConfigError("Failed to validate xml");
	}

	// Free up the parser context
	xmlFreeParserCtxt(ctxt);

	FillTree(xmlDocGetRootElement(doc), config);

	xmlFreeDoc(doc);

	return config;
}


void XMLConfig::Write(const Config& config, std::ostream& os) {

	xmlDocPtr doc = NULL;
	xmlDtdPtr dtd = NULL;
	xmlNodePtr root_node = NULL;
	xmlNodePtr node = NULL;

	std::string root_name = "arc";

	LIBXML_TEST_VERSION;

	// Create a new document, a node and set it as a root node
	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST root_name.c_str());
	xmlDocSetRootElement(doc, root_node);

	// Create DTD declaration.
	dtd = xmlCreateIntSubset(doc,
	                         BAD_CAST root_name.c_str(),
	                         NULL,
	                         BAD_CAST "arc.dtd");

	// Create the content
	for (std::list<ConfGrp>::const_iterator it = config.GetConfigs().begin();
	     it != config.GetConfigs().end(); it++) {

		node = root_node;
		std::string xpath = '/' + root_name;

		// id attribute goes to the second level for infosys options
		int idlevel = (it->GetSection().substr(0, 7) == "infosys") ? 1 : 0;

		std::string::size_type pos = 0;
		while (pos != std::string::npos) {
			std::string::size_type pos2 = it->GetSection().find('/', pos);
			std::string key;
			if (pos2 == std::string::npos) {
				key = it->GetSection().substr(pos);
				pos = std::string::npos;
			}
			else {
				key = it->GetSection().substr(pos, pos2 - pos);
				pos = pos2 + 1;
			}

			xpath += '/' + key;
			if (idlevel == 0) {
				if (!it->GetID().empty())
					xpath += "[@id='" + it->GetID() + "']";
				else
					xpath += "[not(@id)]";
			}

			xmlXPathContextPtr context = xmlXPathNewContext(doc);
			xmlXPathObjectPtr result =
				xmlXPathEvalExpression(BAD_CAST xpath.c_str(), context);
			xmlXPathFreeContext(context);

			// registration blocks are not distinguished by id
			if (xmlXPathNodeSetIsEmpty(result->nodesetval) ||
				key == "registration") {
				node = xmlNewChild(node, NULL, BAD_CAST key.c_str(), NULL);
				if (idlevel == 0 && !it->GetID().empty())
					xmlSetProp(node, BAD_CAST "id",
					           BAD_CAST it->GetID().c_str());
			}
			else
				node = result->nodesetval->nodeTab[0];

			idlevel--;
		}

		for (std::list<Option>::const_iterator i1 = it->GetOptions().begin();
		     i1 != it->GetOptions().end(); i1++) {
			xmlNodePtr optnode = xmlNewChild(node, NULL,
			                                 BAD_CAST i1->GetAttr().c_str(),
			                                 BAD_CAST i1->GetValue().c_str());
			for (std::map<std::string, std::string>::const_iterator
			         i2 = i1->GetSubOptions().begin();
			     i2 != i1->GetSubOptions().end(); i2++) {
				xmlSetProp(optnode,
				           BAD_CAST i2->first.c_str(),
				           BAD_CAST i2->second.c_str());
			}
		}
	}

	xmlChar *xmlbuff;
	int buffersize;
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);

	// Send the buffer to the stream
	os << xmlbuff;

	// Free the buffer
	xmlFree(xmlbuff);

	// Free the document
	xmlFreeDoc(doc);

	// Free variables allocated by parser
	xmlCleanupParser();
}

}
