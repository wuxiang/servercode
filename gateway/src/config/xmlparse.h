#ifndef  XMLPARSE_H_
#define  XMLPARSE_H_
#include <string>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>

class XMLParse {
    public:
        XMLParse(const std::string& filename = "config.xml");
        virtual ~XMLParse();

        std::string getString(const std::string& no, const std::string& field);
        std::set<std::string> getVector(const std::string& no, const std::string& field);

    protected:
        typedef std::map<std::string, std::string>  StringPairMap;
        typedef boost::tuple<StringPairMap, std::map<std::string, std::set<std::string> > >  ObjectTuple;

        void init(const std::string& filename = "config.xml");

        std::map<std::string, ObjectTuple>  m_config;

    private:
        XMLParse(const XMLParse&);

        const std::string empty_string; 
        static const std::set<std::string> null_set;
};

#endif

