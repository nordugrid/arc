/**
 * Interface for handling runtime environments.
 */
#ifndef __AREX2_RTE_H__
#define __AREX2_RTE_H__

#include <string>
#include <vector>

/**
 * RTE class. It represents a runtime environment,
 * and provides functionality for getting information about them.
 */
class RTE {

	public:
	/**
		 * Constructs a new runtime environemt. String should in general
		 * be of the type: STRING-VERSION. Where version consists of numbers
		 * with . between them.
		 */
		RTE(const std::string& re);

		 /**
		  * Destructor. Not that much to say.
		  */
		~RTE();

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
		bool operator==(const RTE& other) const;

		/**
		 * Inequility operator. Return the opsite of ==
		 */
		bool operator!=(const RTE& other) const;

		/**
		 * Greater than operator. Returns true if the compared runtime
		 * environment is greater than the current.
		 */
		bool operator>(const RTE& other) const;

		/**
		 * Less than operator. Returns false if the other is equal,
		 * otherwise it returns the opposite of >
		 */
		bool operator<(const RTE& other) const;

		/**
		 * Greater or equal operator. Returns the opposite of <
		 */
		bool operator>=(const RTE& other) const;

		/**
		 * Less than or equal operator. Returns the oppsite of >
		 */
		bool operator<=(const RTE& other) const;

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

#endif // __AREX2_RTE_H__
