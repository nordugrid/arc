#ifndef ARCLIB_XMLCONFIG
#define ARCLIB_XMLCONFIG

#include <string>

#include "configcore.h"
#include "configio.h"

typedef struct _xmlNode xmlNode;

namespace ARex {

/** Class for reading in configuration files in xml-format. It uses libxml2
 *  for xml-parsing.
 */
class XMLConfig : public ConfigIO {

	public:
		/** Read configuration. */
		Config Read(std::istream& is);

		/** Write configuration. */
		void Write(const Config& config, std::ostream& os);

	private:
		/** Private utility function. */
		void FillTree(xmlNode *node, Config& config);
};

}


#endif // ARCLIB_XMLCONFIG
