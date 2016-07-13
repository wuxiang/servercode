#include "signal_system.h"

namespace Elephants
{
    SignalSystem::SignalSystem(boost::asio::io_service& io_service_): signals(io_service_)
    {

    }

    SignalSystem::~SignalSystem()
    {

    }

    void SignalSystem::registerEvent(const int sig, SignalCallback  callback)
    {
        boost::unique_lock<boost::shared_mutex>  lock(mtx);
        std::map<int, std::deque<SignalCallback> >::iterator it = m_map.find(sig);
        if (it != m_map.end())
        {
            it->second.push_back(callback);
            return;
        }
        m_map[sig].push_back(callback);
        signals.add(sig);
        signals.async_wait(boost::bind(&SignalSystem::handler, this, boost::asio::placeholders::error, boost::asio::placeholders::signal_number));
    }

    void SignalSystem::handler(const boost::system::error_code& e, const int signal_number)
    {
        if (e)
        {
            return;
        }

        boost::shared_lock<boost::shared_mutex>  lock(mtx);
        std::map<int, std::deque<SignalCallback> >::iterator it = m_map.find(signal_number);
        if (it != m_map.end())
        {
            for (std::deque<SignalCallback>::iterator iter = it->second.begin(); iter != it->second.end(); ++iter)
            {
                (*iter)(signal_number);
            }
        }
    }
}



