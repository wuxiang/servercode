#include "log_module.h"

namespace Elephants
{
    CLog& CLog::instance()
    {
	    static CLog  handler;
	    return handler;
    }

    CLog::CLog()
    {
    }

    CLog::~CLog()
    {
    }

    LOG_LEVEL  CLog::reflex(const std::string& level)
    {
        LOG_LEVEL  lv = DEBUG;
        if (level == std::string("DEBUG"))
        {
            lv = DEBUG;
        }
        else if (level == std::string("INFO"))
        {
            lv = INFO;
        }
        else if (level == std::string("NOTICE"))
        {
            lv = NOTICE;
        }
        else if (level == std::string("WARNING"))
        {
            lv = WARNING;
        }
        else if (level == std::string("ERR"))
        {
            lv = ERR;
        }
        else if (level == std::string("CRIT"))
        {
            lv = CRIT;
        }
        else if (level == std::string("ALERT"))
        {
            lv = ALERT;
        }
        else if (level == std::string("EMERG"))
        {
            lv = EMERG;
        }
        else
        {
            lv = DEBUG;
        }

        return lv;
    }


    void CLog::inputHandler(const std::string& name, const std::string& level)
    {
        boost::unique_lock<boost::shared_mutex>  lock(m_mtx);
        std::map<std::string, WHandlerPtr>::iterator it = mFile.find(name);
        if (it == mFile.end())
        {
            mFile.insert(std::make_pair(name, WHandlerPtr(new (std::nothrow) WHanler(name, reflex(level)))));
        }
        else
        {
            it->second->setLogLevel(reflex(level));
        }
    }

    std::string CLog::today()
    {
        time_t timeval;
        const tm    *timeinfo;

        timeval = time(NULL);  
        timeinfo = localtime(&timeval);
        char time_val[50];
        sprintf(time_val,"%04d%02d%02d",
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday
            );
        return std::string(time_val);
    }

    bool  CLog::init(const std::vector<std::string>& module)
    {
	    std::vector<std::string>::const_iterator it = module.begin();
	    while (it != module.end())
	    {
            std::string  module("Hybrid");
            std::string  level("DEBUG");

		    std::string::size_type  pos = it->find(".");
		    if (pos != std::string::npos)
		    {
			    try
			    {
                    module = boost::lexical_cast<std::string>(it->substr(0,pos));
                    level = boost::lexical_cast<std::string>(it->substr(pos + 1));
			    }
			    catch (const boost::bad_lexical_cast&)
			    {
			    }
		    }

            inputHandler(module, level);
		    ++it;
	    }

        return true;
    }


    void  CLog::wLog2File(const std::string& mod, const LOG_LEVEL lev, const std::string&  content)
    {
        std::map<std::string, WHandlerPtr>::const_iterator it = mFile.find(mod);
	    if (it != mFile.end() && it->second)
	    {
		    it->second->inputContent(lev, content);
	    }
    }


    /*****************module handle class***************************/
    CLog::WHanler::WHanler(const std::string&  mod, const LOG_LEVEL lev): level(lev), module(mod), isInit(true)
    {
        direct += module + "_Log";
    #if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	    delimiter = '\\';
    #else
	    delimiter = '/';
    #endif

	    if (!boost::filesystem::exists(direct))
	    {
            if (!boost::filesystem::create_directories(direct))
            {
                isInit = false;
            }
	    }

        if (isInit)
        {
            filename.assign("LOG");
            filename += CLog::today();
            filename += ".log";
            timestamp = CLog::today();

            if (isInit)
            {
                // open file
                std::string  absolutePath(direct + delimiter + filename);
                iFile.open(absolutePath.c_str(), std::ios_base::out | std::ios_base::app);
                if (!iFile.is_open())
                {
                    isInit = false;
                }
            }
        }
    }

    void CLog::WHanler::setLogLevel(const LOG_LEVEL lev)
    {
        boost::lock_guard<boost::mutex>  lock(this->mtx);
        level = lev;
    }


    void CLog::WHanler::inputContent(const LOG_LEVEL lev, const std::string& content)
    {
	    // print log level less than config's level
        boost::lock_guard<boost::mutex>  lock(this->mtx);
	    if ( lev > this->level)
	    {
		    return;
	    }

        if (this->iFile.is_open() && CLog::today() != timestamp)
	    {
		    iFile.flush();
		    iFile.close();
		    isInit = false;
	    }

	    if (!isInit)
	    {
		    filename.assign("LOG");
            filename += CLog::today();
		    filename += ".log";
            timestamp = CLog::today();

		    // open file
		    std::string absolutePath(direct + delimiter + filename);
		    iFile.open(absolutePath.c_str(), std::ios_base::out | std::ios_base::app);
		    if (!iFile.is_open())
		    {
			    isInit = false;
		    }
		    else
		    {
			    isInit = true;
		    }
	    }

	    if (isInit)
	    {
		    iFile << logtime(std::time(NULL)) << ": " << content << "\n";
		    iFile.flush();
	    }

    }

    std::string  CLog::WHanler::logtime(std::time_t  sec)
    {
	    std::tm*  pTime = std::localtime(&sec);
	    std::string str;
	    try
	    {
		    str += boost::lexical_cast<std::string>(pTime->tm_year + 1900);

		    str += "-";
		    if (pTime->tm_mon + 1 < 10)
		    {
			    str += boost::lexical_cast<std::string>(0);
		    }
		    str += boost::lexical_cast<std::string>(pTime->tm_mon + 1);

		    str += "-";
		    if (pTime->tm_mday < 10)
		    {
			    str += boost::lexical_cast<std::string>(0);
		    }
		    str += boost::lexical_cast<std::string>(pTime->tm_mday);

		    str += " ";
		    if (pTime->tm_hour < 10)
		    {
			    str += boost::lexical_cast<std::string>(0);
		    }
		    str += boost::lexical_cast<std::string>(pTime->tm_hour);
		    str += ":";
		    if (pTime->tm_min < 10)
		    {
			    str += boost::lexical_cast<std::string>(0);
		    }
		    str += boost::lexical_cast<std::string>(pTime->tm_min);
		    str += ":";
		    if (pTime->tm_sec < 10)
		    {
			    str += boost::lexical_cast<std::string>(0);
		    }
		    str += boost::lexical_cast<std::string>(pTime->tm_sec);
	    } catch (boost::bad_lexical_cast&) {
		    return std::ctime(&sec);
	    }

	    return  str;
    }


    /***************************************************************************/


    /**************************global log function*****************************/
    void LOG(const std::string& mod, const LOG_LEVEL lev, const char* str,  ...)
    {
	    va_list  args;
	    va_start(args, str);

	    char  buf[4096] = { 0 };
	    vsnprintf(buf, 4096, str, args);
	    std::string  cont(buf);
	    CLog::instance().wLog2File(mod, lev, cont);

	    va_end(args);

    }

    void LOG(const std::string& mod, const LOG_LEVEL lev, const std::string& content)
    {
	    CLog::instance().wLog2File(mod, lev, content);
    }


    void ERRLOG(const char*  fmt,  ...)
    {
        int size=100;
        std::string str;
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
        std::cerr << time << ']' << str << std::endl;
    }

    void ERRLOG(const std::string& content)
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
        std::cerr << time << ']' << content << std::endl;
    }
    /************************************************************************/
}




