#ifndef  XMLPARSE_H_
#define  XMLPARSE_H_
#include <string>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/lexical_cast.hpp>

class XMLParse {
    public:
        XMLParse(const std::string& filename = "config.xml");
        virtual ~XMLParse();

        std::string getString(const std::string& no, const std::string& field);
        std::set<std::string> getVector(const std::string& no, const std::string& field);

        template<typename _T>
        _T getValue(const std::string& no, const std::string& field) {
            std::map<std::string, ObjectTuple>::const_iterator it = m_config.find(no);
            if (it == m_config.end()) {
                return _T();
            }

            const StringPairMap& m = it->second.get<0>();
            StringPairMap::const_iterator ite = m.find(field);
            if (ite == m.end()) {
                return _T();
            }

            return boost::lexical_cast<std::size_t>(ite->second);
        }

        template<char*>
        const char* getValue(const std::string& no, const std::string& field) {
            std::map<std::string, ObjectTuple>::const_iterator it = m_config.find(no);
            if (it == m_config.end()) {
                return NULL;
            }

            const StringPairMap& m = it->second.get<0>();
            StringPairMap::const_iterator ite = m.find(field);
            if (ite == m.end()) {
                return NULL;
            }

            return ite->second.c_str();
        }

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

