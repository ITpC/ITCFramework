/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ThreadPoolManager.h 1 2015-02-28 13:43:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef THREADPOOLMANAGER_H
#define	THREADPOOLMANAGER_H

#include <memory>
#include <stdint.h>
#include <ThreadPool.h>
#include <abstract/Runnable.h>
#include <exception>
#include <sched.h>
#include <sys/Nanosleep.h>
#include <mutex>

namespace itc
{
  struct tpstats
  {
    size_t tqdp;
    size_t tc;
    size_t tpc;
    size_t tac;
    size_t maxthreads;
    float  overcommit;
  };
  /**
   * @brief This class manages an instance of the itc::ThreadPool class. 
   * The default behavior is to expand threads within a itc::ThreadPool instance 
   * logarithmycally based on current value of mMaxThreads attribute of the 
   * itc::ThreadPool if the current value of the attibute mTaskQueue of the 
   * itc::ThreadPool is 25% longer then amount of threads in itc::ThreadPool.
   * The upper limit of number of threads in itc::ThreadPool 
   * is defined by itc::ThreadPoolManager.mMaxThreads + 
   * itc::ThreadPoolManager.mOvercommitThreads. Right now this class is too 
   * simple for soft and hard upper limit, which will be changed later
   * 
   * @TODO Need to add functionality to manage thread pool on time-line and resource
   * usage.
   **/
	class ThreadPoolManager : public abstract::IRunnable
	{
  private:
    std::mutex    mMutex;
    bool          doStart;
    volatile bool doRun;
    volatile bool canStop;
    size_t        mPurgeTm;
    size_t        mMaxThreads;
    size_t        mMinReadyThr;
    size_t        mOvercommitThreads;
    tpstats       mTPStats;
    
	  ::std::shared_ptr<ThreadPool> mThreadPool;

    void stopToggle()
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      canStop=!canStop;
    }
  public:
    explicit ThreadPoolManager(
      const size_t& maxthreads=200,
      const float& overcommit=10,
      const size_t& purge_tm_usec=10000, 
      const size_t& min_thr_ready=10
    ):mMutex(), doStart(false),doRun(true), canStop(true),
      mPurgeTm(purge_tm_usec),mMaxThreads(maxthreads),
      mMinReadyThr(min_thr_ready),mOvercommitThreads(overcommit),
      mThreadPool(std::make_shared<ThreadPool>(min_thr_ready,false,1))
    {
      std::lock_guard<std::mutex> dosync(mMutex);
      doStart=true;
    }
    
    void enqueueRunnable(const abstract::IThreadPool::value_type& ref)
    {
      mThreadPool.get()->enqueue(ref);
    }
    
    
    void execute()
    {
      ::itc::sys::Nap mSleep;
      while(doRun)
      {
        mThreadPool.get()->shakePools();

        mTPStats.tqdp=mThreadPool.get()->getTaskQueueDepth();
        mTPStats.tc=mThreadPool.get()->getThreadsCount();
        mTPStats.tpc=mThreadPool.get()->getPassiveThreadsCount();
        mTPStats.tac=mThreadPool.get()->getActiveThreadsCount();
        mTPStats.maxthreads=mThreadPool.get()->getMaxThreads();
        mTPStats.overcommit=mThreadPool.get()->getOvercommitRatio();

        try
        {
          std::lock_guard<std::mutex> dosync(mMutex);
    
          if(mTPStats.tpc<mMinReadyThr)
          {
            if((mTPStats.tc<(mMaxThreads+mOvercommitThreads)) && (mTPStats.tqdp>(mTPStats.tc*1.25)))
            {
                mThreadPool.get()->expand(size_t(log2(mTPStats.maxthreads)));
            }
          }

          if((mTPStats.tpc>mMinReadyThr)&&(mTPStats.tpc>=mTPStats.tac)&&((mTPStats.tqdp*2)<mTPStats.maxthreads))
          {
            if(mThreadPool.get()->getMaxThreads() > mMaxThreads)
            {
              mThreadPool.get()->reduce(1);
            }
          }
        }catch (std::exception& e)
        {
          throw;
        }
        mSleep.usleep(mPurgeTm);
      }
    }
    
    void logStats()
    {
      ::itc::getLog()->info(
        "tc:%ju  pc:%ju  ac:%ju qd:%ju mt:%ju, mtl:%ju",
        mTPStats.tc, mTPStats.tpc, mTPStats.tac, 
        mTPStats.tqdp,mTPStats.maxthreads,mMaxThreads
      );
    }
    
    void shutdown()
    {
      doRun=false;
    }
    void onCancel()
    {
      doRun=false;
    }
    ~ThreadPoolManager()
    {
      
    }
	};
}

#endif	/* THREADPOOLMANAGER_H */

