/**
 * Interface for handling runtime environments.
 */
#ifndef ARCLIB_RUNTIME_ENVIRONMENT
#define ARCLIB_RUNTIME_ENVIRONMENT

#include <string>
#include <vector>

#include "error.h"

/**
 * RuntimeEnvironment exceptions. Gets thrown when an error occurs
 * regarding a runtime environment.
 */
class RuntimeEnvironmentError : public ARCLibError {
	public:
		/** Standard exception class constructor. */
		RuntimeEnvironmentError(std::string message) : ARCLibError(message) {}
};



/**
 * RuntimeEnvironment class. It represents a runtime environment,
 * and provides functionality for getting information about them.
 */
class RuntimeEnvironment {

	public:
	/**
		 * Constructs a new runtime environemt. String should in general
		 * be of the type: STRING-VERSION. Where version consists of numbers
		 * with . between them.
		 */
		RuntimeEnvironment(const std::string& re);

		 /**
		  * Destructor. Not that much to say.
		  */
		~RuntimeEnvironment();

		/**
		 * Returns a string representation of the runtime environment.
		 * This is usually the same as given in the constructor.
		 */
		std::string str() const;

		/**
		 * Returns the name of the runtime environment.
		 */
		std::string Name() const;

		/**
		 * Returns the version of the runtime environment.
		 */
		std::string Version() const;

		/**
		 * Equliaty operator. Returns true if the runtime environments
		 * have the string representation.
		 */
		bool operator==(const RuntimeEnvironment& other) const;

		/**
		 * Inequility operator. Return the opsite of ==
		 */
		bool operator!=(const RuntimeEnvironment& other) const;

		/**
		 * Greater than operator. Returns true if the compared runtime
		 * environment is greater than the current.
		 */
		bool operator>(const RuntimeEnvironment& other) const;

		/**
		 * Less than operator. Returns false if the other is equal,
		 * otherwise it returns the opposite of >
		 */
		bool operator<(const RuntimeEnvironment& other) const;

		/**
		 * Greater or equal operator. Returns the opposite of <
		 */
		bool operator>=(const RuntimeEnvironment& other) const;

		/**
		 * Less than or equal operator. Returns the oppsite of >
		 */
		bool operator<=(const RuntimeEnvironment& other) const;

	private:
		std::string runtime_environment;
		std::string name;
		std::string version;

		/**
		 * Internally used method for splitting a version string into
		 * numeric tokens.
		 */
		std::vector<std::string> SplitVersion(const std::string& version)
		const;
};

#endif // ARCLIB_RUNTIME_ENVIRONMENT
