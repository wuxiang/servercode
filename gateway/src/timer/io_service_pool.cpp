#include "io_service_pool.h"

namespace Elephants
{
    io_service_pool::io_service_pool(const std::size_t sz): m_size(sz),is_runing(false),io_index(0)
    {
        if (sz == 0)
        {
            throw std::runtime_error("io_service_pool size is zero");
        }

        for (std::size_t i = 0; i < m_size; ++i)
        {
            io_service_sptr  ptr(new (std::nothrow) boost::asio::io_service);
            work_ptr  pWork(new (std::nothrow) boost::asio::io_service::work(*ptr));
            io_service_v.push_back(ptr);
            work_v.push_back(pWork);
        }
    }

     io_service_pool::~io_service_pool()
     {
         if (is_runing)
         {
             stop();
         }
     }

    void io_service_pool::run()
    {
        std::vector<boost::shared_ptr<boost::thread> > threads;

        {
            boost::unique_lock<boost::shared_mutex> lock(mtx4service);
            for (std::size_t i = 0; i < m_size; ++i)
            {
                boost::shared_ptr<boost::thread> th(new (std::nothrow) boost::thread(boost::bind(&boost::asio::io_service::run, io_service_v[i])));
                threads.push_back(th);
            }
        }

        {
            boost::unique_lock<boost::shared_mutex> lock(mtx4run);
            is_runing = true;
        }

        for (std::size_t i = 0; i < m_size; ++i)
        {
            (threads[i])->join();
        }
    }

    void io_service_pool::stop()
    {
        {
            boost::unique_lock<boost::shared_mutex> lock(mtx4run);
            if (!is_runing)
            {
                return;
            }
            is_runing = false;
        }

        boost::unique_lock<boost::shared_mutex> lock(mtx4service);
        for (std::size_t i = 0; i < m_size; ++i)
        {
            (io_service_v[i])->stop();
        }
    }

    boost::asio::io_service& io_service_pool::get_io_service()
    {
        //{
        //    boost::shared_lock<boost::shared_mutex>  lock(mtx4run);
        //    if (!is_runing)
        //    {
        //        throw std::runtime_error("io_service_pool is not runing");
        //    }
        //}
        std::size_t curr = 0;
        {
            boost::unique_lock<boost::shared_mutex> lock(mtx4index);
            if (io_index >= m_size)
            {
                io_index = 0;
            }

            curr = io_index++;
        }

        boost::shared_lock<boost::shared_mutex>  lock(mtx4service);
        return *(io_service_v[curr]);
    }
};

