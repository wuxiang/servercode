/* 
   Log.h
   Copyright (C) 

  Êä³ölog
*/

#include "Log.h"
#include <stdarg.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include "stdio.h"

using namespace std;
using namespace boost::filesystem;

Log *Log::instance_ = NULL;

Log*
Log::Instance()
{
    if(NULL == instance_){
        instance_ = new Log;
    }
    return instance_;
}

Log::~Log()
{
    if(NULL != instance_){
        oFile.flush();
        delete instance_;
    }
}

void
Log::Output(const char* fmt,...)
{
    int size=100;
    string str;
    while (1) {
        str.resize(size);
        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            break;
        }
        else{
            size *=2;
        }
    }
    Output(str);
}

void
Log::Output(std::string& str)
{
    time_t timeval;
    const tm    *timeinfo;

    timeval = time(NULL);  
    timeinfo = localtime(&timeval);

    char time[50];
    sprintf(time,"[%04d%02d%02d_%02d:%02d:%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec
                );
    {
        boost::lock_guard<boost::mutex>  lock(mtx4File);
        long posi = oFile.tellp();
        if (-1 != posi && posi >= FILESIZE)
        {
            oFile.close();
        }

        if (!oFile.is_open())
        {
            char  fName[100] = { 0 };
            sprintf(fName,"%04d%02d%02d%02d%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min
                );
            std::string str("LOG");
            str += std::string(fName) + ".log";
            oFile.open(str.c_str(), std::ios_base::out | std::ios_base::ate);
        }
        oFile << time << ']' << str << std::endl;
        oFile.flush();
    }
    //cout << time << ']' << str << endl;
}

void 
Log::OutputErr(const char* fmt,...)
{
    int size=100;
    string str;
    while (1) {
        str.resize(size);
        va_list ap;
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            break;
        }
        else{
            size *=2;
        }
    }
    OutputErr(str);
}

void
Log::OutputErr(std::string& str)
{
    time_t timeval;
    const tm    *timeinfo;

    timeval = time(NULL);  
    timeinfo = localtime(&timeval);

    char time[50];
    sprintf(time,"[%04d%02d%02d_%02d:%02d:%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec
                );
    boost::mutex::scoped_lock lock(lock_);
    cerr << time << ']' << str << endl;
}


