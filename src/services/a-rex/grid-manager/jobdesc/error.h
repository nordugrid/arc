/**
 * Contains common error message functionality
 *
 */

#ifndef ARCLIB_ERROR
#define ARCLIB_ERROR

#include <exception>
#include <string>

/**
 * This is the top exception for ARCLib. Every exeption in ARCLib should
 * inherit from this. The exception inherits from the top C++ exception:
 * std::exception.
 */
class ARCLibError : public std::exception {
	public:
		/**
		 * Creates a new exception, with the mesage given as argument
		 */
		ARCLibError(std::string message) {
			this->message = message;
		}

		/**
		 * Destructor. Not that much to say.
		 */
		virtual ~ARCLibError() throw() {}

		/**
		 * Returns the message given in the constructer.
		 */
		virtual const char* what() const throw() {
			return message.c_str();
		}

	private:
		std::string message;
};

#endif // ARCLIB_ERROR

