/**************************************************************************
**
** 	Copyright 2011 Duke Inc.
**
**  Author: Xielingjun
**
**************************************************************************/

#ifndef _NB_PROFILER_H_
#define _NB_PROFILER_H_

#include "sys/time.h"

#include <string>
#include <stack>
#include <vector>
#include <map>

#include "ac_tool/nb_stdx_singleton.h"

/* currently disable the profiler */
//#define NB_PROFILER_ON

/* Utilities */
#ifdef NB_PROFILER_ON
#  define NEW_SCOPE_TIMER(name,print)       detail::scope_timer __scope_tm##__LINE__(name,print)
#  define NEW_FUNC_SCOPE_TIMER(print)       detail::scope_timer __func_tm##__LINE__(__FUNCTION__,print)
#  define NEW_MARK_TIMER(name)              detail::mark_timer __mark_tm(name)
#  define TIMER_MARK(sym)                   __mark_tm.mark(sym)
#else
#  define NEW_SCOPE_TIMER(name, print)      ((void)(0))
#  define NEW_FUNC_SCOPE_TIMER(print)       ((void)(0))
#  define NEW_MARK_TIMER(name)              ((void)(0))
#  define TIMER_MARK(sym)                   ((void)(0))
#endif


namespace detail {


struct profiler_record
{
public:
    std::string     name;
    int             count;
    long            total;
    long            avg;

};


class profiler_manager : public boost_singleton<profiler_manager>
{
private:
    typedef std::map<std::string, std::vector<long> > RecordsType;
    typedef RecordsType::iterator RecordsIt;
    RecordsType records_;

    /* for print filter */
    int min_count_;
    long min_total_;
    long min_avg_;

private: 
    profiler_manager();
    ~profiler_manager();

public:
    void insert_record(const std::string& name, long elapse);
    void sort(std::vector<profiler_record>&);
    void print(const std::vector<profiler_record>&);

private:
    static bool record_sort_criterion(const profiler_record&, const profiler_record&);
    void set_print_filter(int min_count, int min_total, int min_avg);
    bool is_print(const profiler_record&);/* print control */

    friend struct boost_singleton<profiler_manager>;
};

    
class mark_timer
{
private:
    std::string     name_;
    int             markCount_;
    struct timeval  start_;
    struct timeval  last_;

public:
    mark_timer(const std::string& name);

    ~mark_timer();

    void mark(const std::string& marker = std::string());

};


class scope_timer
{
private:
    std::string     name_;
    bool            print_;/* if print on destruction */
    struct timeval  start_;

public:
    scope_timer(const std::string& name, bool print = false);

    ~scope_timer();

};

}/* namespace detail */

#endif /* _NB_PROFILER_H_ */
