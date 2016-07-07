/* 
   Config.h
   Copyright (C) 

*/

#ifndef DA_CONFIG_H
#define DA_CONFIG_H

#include <string>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <time.h>
#include <map>

class Config {
public:
    Config(const std::string& filename = "config.ini");
    virtual ~Config();

    bool fail(){return !init_;};

    const std::string& get(const std::string &key);
    const std::vector<std::string>& get_vector(const std::string &key);
    int count(const std::string &key);

protected:
    void init(const std::string& filename);

    bool init_;
    std::map<std::string, std::vector<std::string> > m_setting;

    std::string empty_string; 
    static std::vector<std::string> null_vector;

private:
    Config(const Config&);
};
#endif

