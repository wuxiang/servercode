#ifndef IO_SERVICE_MANAGER_H_
#define IO_SERVICE_MANAGER_H_
#include <boost/thread/shared_mutex.hpp>
#include "io_service_pool.h"


namespace Elephants
{
    class io_service_manager: boost::noncopyable
    {
    public:
        static io_service_manager&  instance();
        bool  init(const std::size_t  sz);
        boost::asio::io_service&  get_io_service();
        void run();
        void stop();

    private:
        io_service_manager();
        ~io_service_manager();

    private:
        typedef boost::shared_ptr<io_service_pool>  io_service_pool_ptr;
        io_service_pool_ptr   pool;

        // init information
        boost::shared_mutex   mtx4init;
        bool  isInited;
    };
};

#endif


