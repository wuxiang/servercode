#include "timer.h"


namespace Elephants
{
    // TimerBase
    TimerBase::TimerBase(CallBackTimer& f):func(f)
    {

    }

    TimerBase::~TimerBase()
    {

    }

    void TimerBase::cancle()
    {

    }


    void TimerBase::run(const boost::system::error_code&  e)
    {
        if (!e)
        {
            func();
            wait_async();
        }
    }

    void TimerBase::wait_async()
    {

    }

    //period Timer
    PeriodTimer::PeriodTimer(boost::asio::io_service& io_service_, std::size_t  period, CallBackTimer  func):TimerBase(func), period_time(period), m_Timer(io_service_, boost::posix_time::seconds(period))
    {
    }

    PeriodTimer::~PeriodTimer()
    {
        //m_Timer.cancel();
    }

    void PeriodTimer::cancle()
    {
        m_Timer.cancel();
    }


    void PeriodTimer::wait_async()
    {
        m_Timer.expires_from_now(boost::posix_time::seconds(period_time));
        m_Timer.async_wait(boost::bind(&PeriodTimer::run, this, boost::asio::placeholders::error));
    }

    // point Timer
    PointerTimer::PointerTimer(boost::asio::io_service& io_service_, const std::string&  exactTime, CallBackTimer  func): TimerBase(func), m_timer(io_service_)
    {
        boost::posix_time::ptime  local_time(boost::gregorian::day_clock::local_day(), boost::posix_time::duration_from_string(exactTime));
        boost::posix_time::ptime  utc = boost::posix_time::second_clock::universal_time();
        boost::posix_time::ptime  local = boost::posix_time::second_clock::local_time();
        if (utc > local)
        {
            boost::posix_time::time_duration  distance = utc - local;
            tZone = local_time + distance;
        }
        else if (utc < local)
        {
            boost::posix_time::time_duration distance = local - utc;
            tZone = local_time - distance;
        }

        if (tZone < utc)
        {
            tZone += boost::gregorian::days(1);
        }

        //std::cout << boost::posix_time::to_simple_string(local_time) << std::endl;
        //std::cout << boost::posix_time::to_simple_string(tZone) << std::endl;
    }

    PointerTimer::~PointerTimer()
    {
        //m_timer.cancel();
    }

    void PointerTimer::cancle()
    {
        m_timer.cancel();
    }


    void PointerTimer::wait_async()
    {
        boost::posix_time::ptime  curr = boost::posix_time::second_clock::universal_time();
        //boost::posix_time::time_duration  tt = curr.time_of_day();
        //boost::posix_time::time_duration  callTime = tZone.time_of_day();
        //if (tt >= callTime)
        if (curr > tZone)
        {
            tZone += boost::gregorian::days(1);
        }
        std::cout << boost::posix_time::to_simple_string(curr) << std::endl;
        std::cout << boost::posix_time::to_simple_string(tZone) << std::endl;

        m_timer.expires_at(tZone);
        m_timer.async_wait(boost::bind(&PointerTimer::run, this, boost::asio::placeholders::error));
    }
}

