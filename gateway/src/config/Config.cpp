/* 
   Config.h
   Copyright (C) 
*/

#include "Config.h"
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <time.h>

std::vector<std::string> Config::null_vector;

Config::Config(const std::string& filename): init_(false) {
    this->init(filename);
}

Config::~Config() {
}

void Config::init(const std::string& filename) {
    fprintf(stderr, "Config::init\n");
    if (access(filename.c_str(), F_OK)) {
        return;
    }

    std::map<std::string, std::vector<std::string> >   data;
    std::ifstream in(filename.c_str(), std::ios::in);
    if(in.fail()){
        fprintf(stderr, "[%s]%s open fail",__FUNCTION__, filename.c_str());
        init_ = false;
        return;
    }

    try{
        std::string line;
        while(!in.eof()){
            std::getline(in,line);
            std::string::size_type pos = line.find("=");
            if(pos == std::string::npos){
                continue;
            }
            std::string title = line.substr(0,pos);
            std::string content = line.substr(pos + 1);
            boost::algorithm::trim(title);
            boost::algorithm::trim(content);

            data[title].push_back(content);
        }
    }
    catch(boost::bad_lexical_cast& e){
        fprintf(stderr, "[%s]%s",__FUNCTION__,e.what());
        init_=false;
        return;
    }

    init_ = true;
    m_setting = data;
}

const std::string& Config::get(const std::string &key) {
    if(m_setting.count(key)){
        return m_setting[key][0];
    }
    return empty_string;
}

const std::vector<std::string>& Config::get_vector(const std::string &key) {
    if(m_setting.count(key)){
        return m_setting[key];
    }
    return null_vector;
}

int Config::count(const std::string &key) {
    if(0 == m_setting.count(key)){
        return 0;
    }
    return m_setting[key].size();
}

