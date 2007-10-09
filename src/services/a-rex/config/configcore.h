/** \file
 * This file describes the core configuration
 */
#ifndef ARCLIB_CONFIGCORE
#define ARCLIB_CONFIGCORE

#include <iostream>
#include <list>
#include <map>
#include <string>

#include <arc/Logger.h>


namespace ARex {

extern Arc::Logger ConfigLogger;

/**
 * Error configuration class.
 */
class ConfigError : public std::exception {
	private:
		std::string message_;

	public:
		/** Constructor for the ConfigError exception. Calls the corresponding
		 *  constructor in ARCLibError.
		 */
		ConfigError(std::string message) : message_(message) {};
		virtual ~ConfigError(void) throw() {};
		std::string& what(void) { return message_; };
};


class Option {
	public:
		Option(const std::string& attr, const std::string& value);
		Option(const std::string& attr, const std::string& value,
			   const std::map<std::string, std::string>& suboptions);
		~Option();
		void AddSubOption(const std::string& attr, const std::string& value);
		std::string FindSubOptionValue(const std::string& attr) const;
		const std::string& GetAttr() const;
		const std::string& GetValue() const;
		const std::map<std::string, std::string>& GetSubOptions() const;
	private:
		std::string attr;
		std::string value;
		std::map<std::string, std::string> suboptions;
};


class ConfGrp {
	public:
		ConfGrp(const std::string& section);
		ConfGrp(const std::string& section, const std::string& id);
		ConfGrp(const std::string& section, const std::string& id,
		        const std::list<Option>& options);
		void AddOption(const Option& opt);
		std::list<Option> FindOption(const std::string& attr) const;
		std::list<std::string> FindOptionValue(const std::string& attr) const;
		const std::string& GetSection() const;
		const std::string& GetID() const;
		const std::list<Option>& GetOptions() const;
	private:
		std::string section;
		std::string id;
		std::list<Option> options;
};


/** Core configuration class. */
class Config {
	public:
		Config();

		~Config();

		void AddConfGrp(const ConfGrp& confgrp);

		/* finds the LAST config group that matches in the section/id. */
		const ConfGrp& FindConfGrp(const std::string& section,
								   const std::string& id) const;

		/** Returns the parsed options. */
		const std::list<ConfGrp>& GetConfigs() const;

		/** Get the configuration values from key. */
		std::list<std::string> ConfValue(const std::string& path) const;

		/** Get the first configuration value from key.
		 *  This is meant as a short cut when it is known that the key is
		 *  not multivalued.
		 */
		std::string FirstConfValue(const std::string& path) const;

	private:
		std::list<ConfGrp> configs;
};


/** Read configuration from input stream trying all known formats */
Config ReadConfig(std::istream& is);


/** Read configuration from file trying all known formats */
Config ReadConfig(const std::string& filename);

}

#endif //  ARCLIB_CONFIGCORE
