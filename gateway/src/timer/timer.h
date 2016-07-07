#ifndef TIMER_H_
#define TIMER_H_
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace Elephants
{
    typedef boost::function<void ()>  CallBackTimer;
    class  TimerBase:boost::noncopyable
    {
    public:
        TimerBase(CallBackTimer& f);
        virtual ~TimerBase();
        virtual void wait_async();
        virtual void cancle();

    protected:
        void run(const boost::system::error_code&  e);

    protected:
        CallBackTimer   func;
    };

    // call function for period times
    class  PeriodTimer: public TimerBase
    {
    public:
        PeriodTimer(boost::asio::io_service& io_service_, std::size_t  period, CallBackTimer  func);
        virtual ~PeriodTimer();
        virtual void wait_async();
        virtual void cancle();

    private:
        std::size_t    period_time;
        boost::asio::deadline_timer   m_Timer;
    };

    // call function at time point everyday
    class PointerTimer: public TimerBase
    {
    public:
        // exactTime's format should like this(xx:xx:xx)
        PointerTimer(boost::asio::io_service& io_service_, const std::string&  exactTime, CallBackTimer  func);
        virtual ~PointerTimer();
        virtual void wait_async();
        virtual void cancle();

    private:
        boost::asio::deadline_timer  m_timer;
        boost::posix_time::ptime  tZone;
    };
}

#endif

