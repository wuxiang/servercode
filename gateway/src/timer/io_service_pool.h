#ifndef IO_SERVICE_POOL_H_
#define IO_SERVICE_POOL_H_
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

namespace Elephants
{
    typedef boost::shared_ptr<boost::asio::io_service>   io_service_sptr;
    typedef boost::shared_ptr<boost::asio::io_service::work>  work_ptr;
    class io_service_pool: boost::noncopyable
    {
    public:
        // this thread will throw exception(std::runtime_error)
        io_service_pool(const std::size_t sz);
        virtual ~io_service_pool();
        void run();
        void stop();
        // get io_service with round_robin
        boost::asio::io_service& get_io_service();

    private:
        std::size_t  m_size;

        // runing flag
        boost::shared_mutex  mtx4run;
        bool is_runing;
        
        boost::shared_mutex  mtx4index;
        std::size_t  io_index;

        boost::shared_mutex  mtx4service;
        std::vector<io_service_sptr>  io_service_v;
        std::vector<work_ptr>  work_v;
    };

};

#endif


