/**************************************************************************
**
** 	Copyright 2011 Duke Inc.
**
**  Author: Xielingjun
**
**************************************************************************/

#include "nb_profiler.h"

#include <iostream>
#include <string>
#include <iomanip>

#include "stdx_log.h"

namespace detail{


profiler_manager::profiler_manager() : min_count_(1), min_total_(0), min_avg_(0)
{
#ifdef NB_PROFILER_ON
    LOG_DEBUG("nb_profiler initializing...");
#endif
}

void profiler_manager::insert_record(const std::string& name, long elapse)
{
    std::map< std::string, std::vector<long> >::iterator it;
    if ((it = records_.find(name)) != records_.end())
    {
        // already has a record
        it->second.push_back(elapse);
    }
    else
    {
        // insert a new record
        std::vector<long> vec(1, elapse);
        records_.insert(std::make_pair(name, vec));
    }
}

/* 
 ** sort criterion : by average
 */
bool profiler_manager::record_sort_criterion(const profiler_record& r1, const profiler_record& r2)
{
    // sort by average time
    return (r1.avg < r2.avg);
}

void profiler_manager::sort(std::vector<profiler_record>& vec)
{
    // fetch all the items from records_
    RecordsIt it = records_.begin();
    for (; it != records_.end(); ++it)
    {
        profiler_record item;
        item.name = it->first;
        item.count = it->second.size();

        item.total = 0;
        std::vector<long>::iterator long_it = it->second.begin();
        for (; long_it != it->second.end(); ++long_it)
            item.total += *long_it;
        item.avg = item.total / item.count;

        vec.push_back(item);
    }

    // sort by name
    std::sort(vec.begin(), vec.end(), this->record_sort_criterion); 
}

/* 
** destructor : print statistics
*/
profiler_manager::~profiler_manager()
{
#ifdef NB_PROFILER_ON
    std::vector<profiler_record> items;
    sort(items);
    print(items);
#endif
}

void profiler_manager::set_print_filter(int min_count, int min_total, int min_avg)
{
    min_count_ = min_count;
    min_total_ = min_total;
    min_avg_ = min_avg;
}

/*
** for print control, filter those trival results 
*/
bool profiler_manager::is_print(const profiler_record& itm)
{
    // print avg > 1ms records
    if( itm.avg > 1000 )
        return true;

    // print total > 10ms records
    if( itm.total > 10000 )
        return true;

    //TODO add other filter controls here
    return false;
}


/*
** print the statistics, called on destruction
*/
void profiler_manager::print(const std::vector<profiler_record>& vec)
{
    std::cout << std::string(100, '=') << std::endl;

    // title
    std::cout << std::left << std::setw(50) << std::setfill(' ') << "Procedure Name";
    std::cout << std::right << std::setw(15) << std::setfill(' ') << "total (us)";
    std::cout << std::right << std::setw(15) << std::setfill(' ') << "n calls";
    std::cout << std::right << std::setw(15) << std::setfill(' ') << "average (us)";
    std::cout << std::endl;
    std::cout << std::string(100, '-') << std::endl;

    // items
    std::vector<profiler_record>::const_iterator it = vec.begin();
    for (; it != vec.end(); ++it)
    {
        if (!is_print(*it))
            continue;
        std::cout << std::left << std::setw(50) << std::setfill(' ') << it->name;
        std::cout << std::right << std::setw(15) << std::setfill(' ') << it->total;
        std::cout << std::right << std::setw(15) << std::setfill(' ') << it->count;
        std::cout << std::right << std::setw(15) << std::setfill(' ') << it->avg;
        std::cout << std::endl;
    }

    std::cout << std::string(100, '=') << std::endl;
    std::cout << std::endl;
}
mark_timer::mark_timer(const std::string& name) : name_(name), markCount_(0)
{
    gettimeofday(&start_, NULL);
    last_ = start_;
}

mark_timer::~mark_timer()
{
    struct timeval end;
    gettimeofday(&end, NULL);

    long elapse = 0;
    elapse += (end.tv_sec - start_.tv_sec) * 1000000;
    elapse += (end.tv_usec - start_.tv_usec);

    profiler_manager::instance().insert_record(name_ + "[TOTAL]", elapse);
}

void mark_timer::mark(const std::string& marker/* = std::string()*/)
{
    // mark and insert a record
    struct timeval cur;
    gettimeofday(&cur, NULL);

    long elapse = 0;
    elapse += (cur.tv_sec - last_.tv_sec) * 1000000;
    elapse += (cur.tv_usec - last_.tv_usec);

    markCount_++;
    profiler_manager::instance().insert_record(name_ + "[" + marker + "]", elapse);

    // record the last
    last_ = cur;
}


scope_timer::scope_timer(const std::string& name, bool print/* = false*/) : name_(name), print_(print)
{
    gettimeofday(&start_, NULL);
}

scope_timer::~scope_timer()
{
    struct timeval end;
    gettimeofday(&end, NULL);

    long elapse = 0;
    elapse += (end.tv_sec - start_.tv_sec) * 1000000;
    elapse += (end.tv_usec - start_.tv_usec);

    profiler_manager::instance().insert_record(name_, elapse);

    if(print_)
        LOG_DEBUG("<<PROFILER>> "<< name_ << " takes " << elapse/1000.0 << " ms");
}


}/* namespace detail */
