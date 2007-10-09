#ifndef ARCLIB_CONFIGIO
#define ARCLIB_CONFIGIO

#include <iostream>

#include "configcore.h"


namespace ARex {

/** Virtual base-class for reading and writing configuration files. Concrete
 *  instances include NGConfig and XMLConfig.
 */
class ConfigIO {
	public:
		virtual ~ConfigIO() {};

		/** Read the named configuration source. */
		virtual Config Read(std::istream& is) = 0;

		/** Write configuration to named configuration destination. */
		virtual void Write(const Config& conf, std::ostream& os) = 0;
};

}

#endif // ARCLIB_CONFIGIO
