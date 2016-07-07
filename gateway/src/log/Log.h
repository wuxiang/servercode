/* 
   Log.h
   Copyright (C) 

  Êä³ö¸÷Ààlog
*/
#ifndef DA_LOG_H
#define DA_LOG_H

#include <list>
#include <string>
#include <fstream>
#include <boost/thread/mutex.hpp>
#define DA_LOG(...) Log::Instance()->Output(__VA_ARGS__)
#define DA_ERRLOG(...) Log::Instance()->OutputErr(__VA_ARGS__)

const long FILESIZE = 1024 * 1024 * 1024;
class Log
{
public:
    static Log *Instance();

    void Output(const char*,...);
    void Output(std::string&);

    void OutputErr(const char*,...);
    void OutputErr(std::string&);
private:
    Log(){};
    virtual ~Log();
    Log(const Log&);

    static Log* instance_;

    std::list<std::string> strbuf_queue_;

    boost::mutex    lock_;
    boost::mutex    mtx4File;
    std::ofstream  oFile;
};


#endif


