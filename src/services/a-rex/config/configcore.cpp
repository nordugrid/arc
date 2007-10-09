/*
 * Core configuration class
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "configcore.h"
#include "ngconfig.h"
#include "xmlconfig.h"

namespace ARex {

Arc::Logger ConfigLogger(Arc::Logger::getRootLogger(), "Config");

Option::Option(const std::string& a, const std::string& v) : attr(a),
                                                             value(v) {}

Option::Option(const std::string& a, const std::string& v,
               const std::map<std::string, std::string>& s) : attr(a),
                                                              value(v),
                                                              suboptions(s) {}

Option::~Option() {}

void Option::AddSubOption(const std::string& attr, const std::string& value) {
	suboptions[attr] = value;
}

std::string Option::FindSubOptionValue(const std::string& attr) const {
	std::map<std::string, std::string>::const_iterator
		i = suboptions.find(attr);
	return (i == suboptions.end() ? std::string() : i->second);
}

const std::string& Option::GetAttr() const {
	return attr;
}

const std::string& Option::GetValue() const {
	return value;
}

const std::map<std::string, std::string>& Option::GetSubOptions() const {
	return suboptions;
}


ConfGrp::ConfGrp(const std::string& s) : section(s) {}

ConfGrp::ConfGrp(const std::string& s,
                 const std::string& i) : section(s), id (i) {}

ConfGrp::ConfGrp(const std::string& s, const std::string& i,
                 const std::list<Option>& o) : section(s), id (i),
                                               options(o) {}

void ConfGrp::AddOption(const Option& opt) {
	options.push_back(opt);
}

std::list<Option> ConfGrp::FindOption(const std::string& attr) const {
	std::list<Option> opts;
	for (std::list<Option>::const_iterator it = options.begin();
	     it != options.end(); it++)
		if (it->GetAttr() == attr)
			opts.push_back(*it);
	return opts;
}

std::list<std::string> ConfGrp::FindOptionValue(const std::string& attr) const {
	std::list<std::string> opts;
	for (std::list<Option>::const_iterator it = options.begin();
	     it != options.end(); it++)
		if (it->GetAttr() == attr)
			opts.push_back(it->GetValue());
	return opts;
}

const std::string& ConfGrp::GetSection() const {
	return section;
}

const std::string& ConfGrp::GetID() const {
	return id;
}

const std::list<Option>& ConfGrp::GetOptions() const {
	return options;
}


Config::Config() {}

Config::~Config() {}

void Config::AddConfGrp(const ConfGrp& confgrp) {
	configs.push_back(confgrp);
}

const ConfGrp& Config::FindConfGrp(const std::string& section,
                                   const std::string& id) const {
	for (std::list<ConfGrp>::const_reverse_iterator it = configs.rbegin();
	     it != configs.rend(); it++)
		if (it->GetSection() == section && it->GetID() == id)
			return *it;
	throw ConfigError("Configuration group not defined");
}

const std::list<ConfGrp>& Config::GetConfigs() const {
	return configs;
}

std::list<std::string> Config::ConfValue(const std::string& path) const {
	try {
		std::string id;
		std::string::size_type pos1 = path.find('@');
		if (pos1 != std::string::npos)
			id = path.substr(0, pos1++);
		else
			pos1 = 0;
		std::string::size_type pos2 = path.rfind('/');
		if (pos2 == std::string::npos || pos2 < pos1)
			throw ConfigError("Illegal configuration path");
		return FindConfGrp(path.substr(pos1, pos2 - pos1),
		                   id).FindOptionValue(path.substr(pos2 + 1));
	}
	catch(ConfigError) {
		return std::list<std::string>();
	}
}


std::string Config::FirstConfValue(const std::string& path) const {
	std::list<std::string> values = ConfValue(path);
	if (!values.empty())
		return values.front();
	else
		return std::string();
}


Config ReadConfig(std::istream& is) {

	try {
		is.seekg(0, std::ios::beg);
		return XMLConfig().Read(is);
	}
	catch(ConfigError) {
		is.clear();
	}
	try {
		is.seekg(0, std::ios::beg);
		return NGConfig().Read(is);
	}
	catch(ConfigError) {
		is.clear();
	}
	throw ConfigError("Unknown configuration format");
}


Config ReadConfig(const std::string& filename) {

	static std::map<std::string, Config> configcache;

	if (configcache.find(filename) != configcache.end()) {
		ConfigLogger.msg(Arc::VERBOSE, "Using cached configuration: %s", filename.c_str());
		return configcache[filename];
	}

	ConfigLogger.msg(Arc::VERBOSE, "Reading configuration file: %s", filename.c_str());

	std::ifstream is(filename.c_str());
	Config conf = ReadConfig(is);
	is.close();
	configcache[filename] = conf;
	return conf;
}

}

