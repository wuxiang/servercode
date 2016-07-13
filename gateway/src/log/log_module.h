#ifndef _CLOG_H_
#define _CLOG_H_
#include <fstream>
#include <set>
#include <map>
#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <cstdarg>
#include <cstdio>


namespace Elephants
{
    enum LOG_LEVEL
    {
	    EMERG		= 0,   /* system is unusable */
	    ALERT		= 1,   /* action must be taken immediately */
	    CRIT		= 2,   /* critical conditions */
	    ERR			= 3,   /* error conditions */
	    WARNING		= 4,   /* warning conditions */
	    NOTICE		= 5,   /* normal but significant condition */
	    INFO		= 6,   /* informational */
	    DEBUG		= 7,   /* debug-level messages */
    };

    extern void LOG(const std::string& mod, const LOG_LEVEL lev, const char*  str,  ...);
    extern void LOG(const std::string& mod, const LOG_LEVEL lev, const std::string& content);
    extern void ERRLOG(const char*  fmt,  ...);
    extern void ERRLOG(const std::string& content);


    class CLog
    {
    public:
	    static CLog& instance();
	    bool  init(const std::vector<std::string>& module);
        void  wLog2File(const std::string& mod, const LOG_LEVEL lev, const std::string&  content);

        static LOG_LEVEL  reflex(const std::string& level);
        static std::string today();

    private:
	    CLog();
	    ~CLog();
	    CLog(const CLog&);
	    const CLog& operator=(const CLog&);
        void inputHandler(const std::string& name,  const std::string& level);

    protected:
	    class  WHanler
	    {
	    public:
            WHanler(const std::string&  mod, const LOG_LEVEL lev, const int backup = 10);
		    void inputContent(const LOG_LEVEL lev, const std::string& content);
            void setLogLevel(const LOG_LEVEL lev);

	    private:
		    std::string  logtime(std::time_t  sec);
		    WHanler(const WHanler&);

	    private:
		    bool  isInit;
		    LOG_LEVEL   level;
            std::string   module;

		    std::string  direct;  // 目录
		    std::string  delimiter; //分隔符
            std::string  timestamp; // 当前文件的时间戳
		    std::string  filename;  //文件名
            int          backup;    // 保留日志文件天数, -1表示永远不删除

		    boost::mutex  mtx;
		    std::fstream  iFile;
	    };

	    typedef boost::shared_ptr<WHanler>   WHandlerPtr;

    private:
        boost::shared_mutex   m_mtx;
        std::map<std::string, WHandlerPtr>  mFile;
    };

}

#define DA_LOG(...) Elephants::LOG(__VA_ARGS__)
#define DA_ERRLOG(...) Elephants::ERRLOG(__VA_ARGS__)

#endif //_CLOG_H_


