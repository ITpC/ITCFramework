/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: RScheduler.h 22 2015-02-16 19:00:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/


#ifndef RSCHEDULER_H
#  define	RSCHEDULER_H


#  include <stdint.h>
#  include <memory>
#  include <map>
#  include <string>
#  include <list>
#  include <abstract/Runnable.h>
#  include <InterfaceCheck.h>
#  include <cmath>
#  include <Date.h>
#  include <Singleton.h>
#  include <sched.h>
#  include <ThreadPool.h>
#  include <DateFormatter.h>
#  include <sys/Nanosleep.h>
#  include <limits>
#  include <mutex>
#  include <atomic>

typedef itc::utils::Date Date;
typedef itc::utils::Time Time;
typedef itc::utils::DateFormatter DateFormatter;


namespace itc
{

  /**
   * @brief implements scheduling of tasks execution at specific timepoint.
   * With this class it is possible to schedule a task to be executed within 
   * some miliseconds after call to RScheduler::add() method.
   * The execution of a task is scheduled to happen at the specific time,
   * however it may be delayed by 100 microseconds or a little longer. The 
   * delay over 100 microseconds depends on a granularity of a system 
   * timers.
   **/
  class RScheduler : public abstract::IRunnable
  {
   private:
    typedef ::std::shared_ptr<abstract::IRunnable> storable;
    typedef ::std::list<storable> storable_container;
    typedef typename storable_container::iterator scIterator;
    typedef ::std::map<Date, storable_container> map_type;
    typedef typename map_type::iterator mapIterator;
    typedef ::itc::ThreadPool ThreadPoolType;
    typedef ::std::shared_ptr<ThreadPoolType> ThreadPoolPointer;


    std::mutex mMutex;
    std::atomic<bool> mDoRun;
    std::atomic<bool> mMayAdd;
    Time mNextWake;
    map_type mSchedule;
    size_t mShakePoolsTO;
    ThreadPoolPointer mThreadPool;

   public:

    explicit RScheduler(size_t maxthreads = 5, float overcommit = 5)
      : mDoRun(true), mMayAdd(true), mShakePoolsTO(10000),
      mThreadPool(std::make_shared<ThreadPoolType>(
      maxthreads, false, overcommit
      )
      )
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      itc::getLog()->debug(__FILE__, __LINE__, "RScheduler::RScheduler()");
    }

    inline pthread_t getThreadId()
    {
      return ::pthread_self();
    }

    const void add(uint32_t msoffset, const storable& ref)
    {
      std::lock_guard<std::mutex> dosync(mMutex);

      if(mDoRun && mMayAdd)
      {
        itc::getLog()->debug(__FILE__, __LINE__, "in -> RScheduler::add()");
        Date aDate;
        Time aTime(aDate.getTime());
        time_t usec = aTime.mTimestamp.tv_usec + msoffset * 1000;
        time_t sec = usec / 1000000;

        aTime.mTimestamp.tv_sec += sec;
        aTime.mTimestamp.tv_usec -= (sec * 1000000);

        aDate.set(aTime);

        mapIterator it = mSchedule.find(aDate);

        if(it != mSchedule.end())
        {
          it->second.push_back(ref);
        }else
        {
          storable_container tmp;
          tmp.push_back(ref);
          mSchedule.insert(std::pair<Date, storable_container>(aDate, tmp));
        }

        mNextWake = mSchedule.begin()->first.getTime();
        itc::getLog()->debug(__FILE__, __LINE__, "out <- RScheduler::add()");
      }
    }

    bool mayRun()
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      return mDoRun;
    }

    void execute()
    {
      ::itc::sys::Nap waithere;
      while(mayRun())
      {
        try
        {
          std::lock_guard<std::mutex> dosync(mMutex);
          if(!mSchedule.empty())
          {
            // itc::getLog()->trace(__FILE__,__LINE__,"in -> RScheduler::execute() mSchedule is not empty");
            if(mNextWake > mSchedule.begin()->first.getTime())
            {
              mNextWake = mSchedule.begin()->first.getTime();
            }

            Date aDate;

            if(aDate.getTime() >= mNextWake)
            {
              scIterator it = mSchedule.begin()->second.begin();
              scIterator itend = mSchedule.begin()->second.end();

              while(it != itend)
              {
                itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] RScheduler::execute() about to be enqueued", pthread_self());
                mThreadPool->enqueue(*it);
                itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] RScheduler::execute() has been enqueued", pthread_self());
                sched_yield(); // let the thread start;
                mapIterator it1 = mSchedule.begin();
                itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] RScheduler::execute() before remove from map", pthread_self());
                mSchedule.erase(it1);
                itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] RScheduler::execute() after remove from map", pthread_self());
                if(mSchedule.begin() == mSchedule.end())
                {
                  break;
                }else
                {
                  it = mSchedule.begin()->second.begin();
                }
              }

              if(mSchedule.begin() != mSchedule.end())
              {
                mNextWake = mSchedule.begin()->first.getTime();
              }else
              {
                Time future(std::numeric_limits<time_t>::max(), std::numeric_limits<time_t>::max());
                mNextWake = future;
              }
            }
            //itc::getLog()->trace(__FILE__,__LINE__,"out <- RScheduler::execute() mSchedule is not empty");
          }
        }catch(std::exception& e)
        {
          throw e; //rethrow
        }
        waithere.usleep(100);
        mThreadPool->shakePools();
      }
    }

    const bool isScheduleEmpty()
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      return mSchedule.empty();
    }

    void onCancel()
    {
      stopRunning();
      this->shutdown();
    }

    void shutdown()
    {
      itc::getLog()->debug(__FILE__, __LINE__, "trace -> in -> RScheduler::shutdown()");
      stopAdd();
      while(!isScheduleEmpty())
      {
        clearSchedule();
        sched_yield();
      }
      itc::getLog()->debug(__FILE__, __LINE__, "trace <- out <- RScheduler::shutdown()");
    }

   private:

    void stopAdd()
    {
      mMayAdd = false;
    }

    void stopRunning()
    {
      mDoRun = false;
    }

    void clearSchedule()
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      itc::getLog()->debug(__FILE__, __LINE__, "trace <- out <- RScheduler::clearSchedule()");
      mSchedule.clear();
    }
  };
}


std::shared_ptr<itc::RScheduler> getRScheduler();

#endif	/* RSCHEDULER_H */

