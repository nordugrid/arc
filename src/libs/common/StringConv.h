#ifndef ARCLIB_STRINGCONV
#define ARCLIB_STRINGCONV

#include <iomanip>
#include <sstream>
#include <string>

#include "Logger.h"


namespace Arc {


extern Logger stringLogger;


/** This method converts a string to any type
 */
template<typename T>
T stringto(const std::string& s) {
	T t;
	if (s.empty()) {
		stringLogger.msg(ERROR, "Empty String");
		return 0;
	}
	std::stringstream ss(s);
	ss >> t;
	if (ss.fail()) {
		stringLogger.msg(ERROR, "Conversion failed");
		return 0;
	}
	if (!ss.eof())
		stringLogger.msg(WARNING, "Full string not used");
	return t;
}


#define stringtoi(A) stringto<int>((A))
#define stringtoui(A) stringto<unsigned int>((A))
#define stringtol(A) stringto<long>((A))
#define stringtoll(A) stringto<long long>((A))
#define stringtoul(A) stringto<unsigned long>((A))
#define stringtoull(A) stringto<unsigned long long>((A))
#define stringtof(A) stringto<float>((A))
#define stringtod(A) stringto<double>((A))
#define stringtold(A) stringto<long double>((A))


/** This method converts a long to any type of the width given. */
template<typename T>
std::string tostring(T t, const int width = 0) {
	std::stringstream ss;
	ss << std::setw(width) << t;
	return ss.str();
}

} // namespace Arc

#endif // ARCLIB_STRINGCONV
