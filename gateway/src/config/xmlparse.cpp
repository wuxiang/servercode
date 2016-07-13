#include "xmlparse.h"

#include <unistd.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

const std::set<std::string>  XMLParse::null_set;

XMLParse::XMLParse(const std::string& filename) {
    this->init(filename);
}

XMLParse::~XMLParse() {
}

void XMLParse::init(const std::string& filename) {
    fprintf(stderr, "XMLParse::init\n");
    if (access(filename.c_str(), F_OK)) {
        return;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::xml_parser::read_xml(filename, pt);

    // iterator   [string, ptree]
    typedef boost::property_tree::ptree::const_iterator  PTreeIterator;
    // get gateway
    boost::property_tree::ptree gateway = pt.get_child("configuration.gateway");
    ObjectTuple   gcf;
    for (PTreeIterator it = gateway.begin(); it != gateway.end(); ++it) {
        if (0 == it->second.size()) {
            StringPairMap&  m = gcf.get<0>();
            m[it->first] = it->second.data();
        }
    }

    m_config[(gcf.get<0>())["NO"]] = gcf;

    // get application
    boost::property_tree::ptree application = pt.get_child("configuration.application");
    for (PTreeIterator it = application.begin(); it != application.end(); ++it) {
        if ("service" == it->first && it->second.size()) {
            ObjectTuple  service;
            StringPairMap&  m = service.get<0>();
            std::map<std::string, std::set<std::string> >& ms = service.get<1>();
            for (PTreeIterator ite = it->second.begin(); ite != it->second.end(); ++ite) {
                if (0 == ite->second.size()) {
                    m[ite->first] = ite->second.data();
                } else {
                    std::set<std::string>  servers;
                    for (PTreeIterator iter = ite->second.begin(); iter != ite->second.end(); ++iter) {
                        if (0 == iter->second.size()) {
                            servers.insert(iter->second.data());
                        }
                    }
                    ms[ite->first] = servers;
                }
            }

            m_config[m["NO"]] = service;
        }
    }

    // print information
    //for (std::map<std::string, ObjectTuple>::const_iterator it = m_config.begin(); it != m_config.end(); ++it) {
    //    fprintf(stderr, "\nextern key: %s\n", it->first.c_str());
    //    const StringPairMap& m = it->second.get<0>();
    //    for (StringPairMap::const_iterator ite = m.begin(); ite != m.end(); ++ite) {
    //        fprintf(stderr, "%s->%s\n", ite->first.c_str(), ite->second.c_str());
    //    }

    //    const std::map<std::string, std::set<std::string> >& ms = it->second.get<1>();
    //    std::map<std::string, std::set<std::string> >::const_iterator iter = ms.begin();
    //    for (; iter != ms.end(); ++iter) {
    //        fprintf(stderr, "\nkey: %s, values:", iter->first.c_str());
    //        for (std::set<std::string>::const_iterator is = iter->second.begin(); is != iter->second.end(); ++is) {
    //            fprintf(stderr, "%s\t", is->c_str());
    //        }
    //    }
    //}
}

std::string XMLParse::getString(const std::string& no, const std::string& field) {
    std::map<std::string, ObjectTuple>::const_iterator it = m_config.find(no);
    if (it == m_config.end()) {
        return empty_string;
    }

    const StringPairMap& m = it->second.get<0>();
    StringPairMap::const_iterator ite = m.find(field);
    if (ite == m.end()) {
        return empty_string;
    }

    return ite->second;
}

std::set<std::string> XMLParse::getVector(const std::string& no, const std::string& field) {
    std::map<std::string, ObjectTuple>::const_iterator it = m_config.find(no);
    if (it == m_config.end()) {
        return null_set;
    }

    const std::map<std::string, std::set<std::string> >& ms = it->second.get<1>();
    std::map<std::string, std::set<std::string> >::const_iterator iter = ms.find(field);
    if (iter == ms.end()) {
        return null_set;
    }

    return iter->second;
}
