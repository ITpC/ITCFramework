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
#include <cmath>
#include <atomic>

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
   * logarithmically based on current value of mMaxThreads attribute of the 
   * itc::ThreadPool, if current value of the attribute mTaskQueue of the 
   * itc::ThreadPool is 25% larger then amount of threads in itc::ThreadPool.
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
    itc::sys::AtomicMutex       mMutex;
    bool                        doStart;
    std::atomic<bool>           doRun;
    std::atomic<bool>           canStop;
    size_t                      mPurgeTm;
    size_t                      mMaxThreads;
    size_t                      mMinReadyThr;
    size_t                      mOvercommitThreads;
    tpstats                     mTPStats;
	  std::shared_ptr<ThreadPool> mThreadPool;
    itc::sys::Nap               mSleep;
    
  public:
    explicit ThreadPoolManager(
      const size_t& maxthreads=200,
      const size_t& overcommit=10,
      const size_t& purge_tm_usec=10000, 
      const size_t& min_thr_ready=10
    ):mMutex(), doStart(false),doRun(true), canStop(true),
      mPurgeTm(purge_tm_usec),mMaxThreads(maxthreads),
      mMinReadyThr(min_thr_ready),mOvercommitThreads(overcommit),
      mThreadPool(std::make_shared<ThreadPool>(min_thr_ready,false,1))
    {
      AtomicLock dosync(mMutex);
      doStart=true;
      if(mMaxThreads>10000)
      {
        throw std::logic_error("ThreadPoolManager maximum threads hard limit is 10000");
      }
    }
    
    void enqueueRunnable(const abstract::IThreadPool::value_type& ref)
    {
      mThreadPool.get()->enqueue(ref);
    }
    
    
    void execute()
    {
      while(doRun)
      {
        canStop=false;
        mThreadPool.get()->shakePools();

        mTPStats.tqdp=mThreadPool.get()->getTaskQueueDepth();
        mTPStats.tc=mThreadPool.get()->getThreadsCount();
        mTPStats.tpc=mThreadPool.get()->getPassiveThreadsCount();
        mTPStats.tac=mThreadPool.get()->getActiveThreadsCount();
        mTPStats.maxthreads=mThreadPool.get()->getMaxThreads();
        mTPStats.overcommit=mThreadPool.get()->getOvercommitRatio();

        try
        {
          AtomicLock dosync(mMutex);
    
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
      canStop=true;
    }
    
    void logStats()
    {
      ::itc::getLog()->info(
      __FILE__,__LINE__,
        "tc:%ju  pc:%ju  ac:%ju qd:%ju mt:%ju, mtl:%ju",
        mTPStats.tc, mTPStats.tpc, mTPStats.tac, 
        mTPStats.tqdp,mTPStats.maxthreads,mMaxThreads
      );
    }
    
    void shutdown()
    {
      doRun=false;
      itc::sys::Nap aSleep;
      while(!canStop)
      {
        aSleep.usleep(10000);
      };
      mThreadPool->stopPool();
    }
    void onCancel()
    {
      this->shutdown();
    }
    ~ThreadPoolManager()
    {
      this->shutdown();
    }
	};
}

#endif	/* THREADPOOLMANAGER_H */

