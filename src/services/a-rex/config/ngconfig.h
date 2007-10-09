#ifndef ARCLIB_NGCONFIG
#define ARCLIB_NGCONFIG

#include <string>

#include "configcore.h"
#include "configio.h"

namespace ARex {

/** Configuration class used for reading configuration files ARC-style. */
class NGConfig : public ConfigIO {
	public:
		/** Read old arc.conf style configuration. */
		Config Read(std::istream& is);

		/** Write configuration to named file. */
		void Write(const Config& config, std::ostream& os);

	private:
		/** Private utility function. */
		void WriteOption(const Option& opt, std::ostream& os);
};

}

#endif // ARCLIB_NGCONFIG
