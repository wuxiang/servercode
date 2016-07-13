#ifndef SIGNAL_SYSTEM_H
#define SIGNAL_SYSTEM_H
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <deque>

namespace Elephants
{
    typedef  boost::function<void (const int)>   SignalCallback;
    class SignalSystem
    {
    public:
        SignalSystem(boost::asio::io_service& io_service_);
        ~SignalSystem();
        void registerEvent(const int sig, SignalCallback  callback);

    private:
        void handler(const boost::system::error_code& e, const int signal_number);

    private:
        boost::shared_mutex  mtx;
        std::map<int, std::deque<SignalCallback> >  m_map;
        boost::asio::signal_set  signals;
    };
}


#endif


