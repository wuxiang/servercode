#include "xmlparse.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

const std::set<std::string>  XMLParse::null_set;

XMLParse::XMLParse(const std::string& filename) {
    this->init(filename);
}

XMLParse::~XMLParse() {
}

void XMLParse::init(const std::string& filename) {
    boost::property_tree::ptree pt;
    boost::property_tree::xml_parser::read_xml(filename, pt);

    typedef boost::property_tree::ptree::iterator  PTreeIterator;
    // get gateway
    boost::property_tree::ptree gateway = pt.get_child("configuration.gateway");
    for (PTreeIterator it = gateway.begin(); it != gateway.end(); ++it) {
    }

    // get application
    boost::property_tree::ptree application = pt.get_child("configuration.application");
    for (PTreeIterator it = application.begin(); it != application.end(); ++it) {
    }
}

std::string XMLParse::getString(const std::string& no, const std::string& field) {
    return empty_string;
}

std::set<std::string> XMLParse::getVector(const std::string& no, const std::string& field) {
    return null_set;
}
