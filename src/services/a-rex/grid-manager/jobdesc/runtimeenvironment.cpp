#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <stdlib.h>

#include "runtimeenvironment.h"
#include <arc/StringConv.h>

RuntimeEnvironment::RuntimeEnvironment(const std::string& re) {

	runtime_environment = re;

	name = runtime_environment;
	version = "";

	/* This has some assumptions, but holds for all the sane ones */
	std::string::size_type pos = re.find_first_of(" -");
	while ((pos != std::string::npos) && !(isdigit(re[pos+1])))
		pos = re.find_first_of(" -", pos+1);
	if (pos != std::string::npos) {
		name = re.substr (0, pos);
		version = re.substr(pos+1);
	}
}


RuntimeEnvironment::~RuntimeEnvironment() { }


std::string RuntimeEnvironment::str() const {
	return runtime_environment;
}


std::string RuntimeEnvironment::Name() const {
	return name;
}


std::string RuntimeEnvironment::Version() const {
	return version;
}


bool RuntimeEnvironment::operator==(const RuntimeEnvironment& other) const {
	if (runtime_environment==other.str())
		return true;
	else
		return false;
}


bool RuntimeEnvironment::operator!=(const RuntimeEnvironment& other) const {
	return !(*this == other);
}


bool RuntimeEnvironment::operator>(const RuntimeEnvironment& other) const {

	if (name!=other.Name()) return (name>other.Name());

	// Get version
	std::vector<std::string> my_version = SplitVersion(version);
	std::vector<std::string> other_version = SplitVersion(other.Version());

	// Ensure common length
	int ml = my_version.size();
	int ol = other_version.size();
	unsigned int max_size = std::max(ml, ol);

	while (my_version.size()<max_size)
		my_version.push_back("0");
	while (other_version.size()<max_size)
		other_version.push_back("0");

	// Start comparing
	for (unsigned int i=0; i<max_size; i++) {
		if (my_version[i]==other_version[i]) continue;
		// first try to compare as integers
                char* my_e;
                char* other_e;
                int my_v = strtol(my_version[i].c_str(),&my_e,0);
                int other_v = strtol(other_version[i].c_str(),&other_e,0);
                if((*my_e == 0) && (*other_e == 0)) return (my_v > other_v);
		// otherwise compare as strings
		return my_version[i]>other_version[i];
	}

	// If we are here the versions are the same.
	return false;
}


bool RuntimeEnvironment::operator<(const RuntimeEnvironment& other) const {
	if (*this == other) return false;
	return !(*this > other);
}


bool RuntimeEnvironment::operator>=(const RuntimeEnvironment& other) const {
	return !(*this < other);
}


bool RuntimeEnvironment::operator<=(const RuntimeEnvironment& other) const {
	return !(*this > other);
}


std::vector<std::string> RuntimeEnvironment::SplitVersion(const std::string&
version) const {

	std::vector<std::string> tokens;
	if (version.empty()) return tokens;

	std::string::size_type pos = 0;
	std::string::size_type last = 0;

	while ((pos = version.find_first_of(".-", last)) != std::string::npos) {
		std::string token = version.substr(last, pos-last);
		tokens.push_back(token);
		last = pos + 1;
	}

	std::string lasttoken = version.substr(last, version.size()-last);
	tokens.push_back(lasttoken);

	return tokens;
}
