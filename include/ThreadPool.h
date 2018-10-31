/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: ThreadPool.h 22 2010-11-23 12:53:33Z pk $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#include <memory>
#include <Val2Type.h>
// #include <bdmap.h>
#include <list>
#include <algorithm>
#include <abstract/Runnable.h>
#include <abstract/IThreadPool.h>
#include <sys/PThread.h>
#include <TSLog.h>
#include <queue>
#include <Date.h>
#include <mutex>
#include <atomic>
#include <sys/mutex.h>
#include <sys/synclock.h>



#ifndef __THREADPOOL_H__
#  define    __THREADPOOL_H__


namespace itc
{

  /**
   * @brief This class is a container for task queues and it does not manage
   * the threads. It is capable to create and invoke threads on task enqueue,
   * however after the task is done, the thread persists in mActiveThreads 
   * container and will not be moved to mPassiveThreads queue.
   * You does not need this class without a itc::ThreadPoolManager. So instantiate
   * itc::ThreadPoolManager, which will create an instance of itc::ThreadPool 
   * within itself and will effectively manage the threads in a pool.
   **/
  class ThreadPool : public abstract::IThreadPool
  {
   public:
    typedef ::itc::sys::PThread::TaskType TaskType;
    typedef std::shared_ptr<sys::PThread> ThreadPTR;
    typedef std::list<ThreadPTR>::iterator ThreadListIterator;

    explicit ThreadPool(
      const size_t maxthreads = 10, bool autotune = true, float overcommit = 1.2
      ) : mMutex(), mMaxThreads(maxthreads), mMinThreads(maxthreads), 
      mAutotune(autotune), mOvercommitRatio(overcommit), doRun(true),
      mInQueueDepth{0}
    {
      ITCSyncLock dosync(mMutex);
      ::itc::getLog()->debug(
        __FILE__, __LINE__,
        "created ThreadPool::ThreadPool(%ju,%u,%f)",
        size_t(mMaxThreads), bool(mAutotune), float(mOvercommitRatio)
        );
      spawnThreads(mMaxThreads);
    }

    const bool getAutotune() const
    {
      return mAutotune;
    }

    void setAutotune(const bool& autotune)
    {
      ITCSyncLock dosync(mMutex);
      mAutotune = autotune;
    }

    const size_t getMaxThreads() const
    {
      return mMaxThreads;
    }

    const float getOvercommitRatio() const
    {
      return mOvercommitRatio;
    }

    const size_t getThreadsCount() const
    {
      return mActiveThreads.size() + mPassiveThreads.size();
    }

    const size_t getActiveThreadsCount() const
    {
      return mActiveThreads.size();
    }

    const size_t getPassiveThreadsCount() const
    {
      return mPassiveThreads.size();
    }

    const bool mayRun() const
    {
      return doRun;
    }

    void expand(const size_t& inc)
    {
      ITCSyncLock dosync(mMutex);
      if(mayRun())
      {
        mMaxThreads += inc;
        for(size_t i = 0; i < inc; i++)
          mPassiveThreads.push(std::make_shared<sys::PThread>());
      }
    }

    void reduce(const size_t& dec)
    {
      ITCSyncLock dosync(mMutex);
      if(mMaxThreads > mMinThreads)
      {
        mMaxThreads -= dec;
      }
    }

    const size_t getFreeThreadsCount()
    {
      ITCSyncLock dosync(mMutex);
      size_t ftc = 0;
      std::for_each(
        mActiveThreads.begin(),
        mActiveThreads.end(),
        [&ftc](const std::shared_ptr<itc::sys::PThread>& sptr){
          if(sptr.get()->getState() == DONE)
          {
            ftc++;
          }
        }
      );
      return mPassiveThreads.size() + ftc;
    }

    void shakePools()
    {
      ITCSyncLock dosync(mMutex);

      if(mayRun())
      {
        shakePoolsPrivate();

        if(mPassiveThreads.empty()&&(!mTaskQueue.empty()) && mAutotune)
        {
          size_t absMax = (size_t) (mMaxThreads * mOvercommitRatio);
            
          size_t max_start = absMax - getThreadsCount();

          spawnThreads(max_start);
        }
        while((!mPassiveThreads.empty())&&(!mTaskQueue.empty()))
        {
          enqueuePrivate();
        }
      }
    }

    void enqueue(const value_type& ref)
    {
      ITCSyncLock dosync(mMutex);

      if(mayRun())
      {
        mTaskQueue.push(ref);
        mInQueueDepth++;
        itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] ThreadPool::enqueue() the Runnable is enqueued", pthread_self());
        if(!mPassiveThreads.empty())
        {
          itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] ThreadPool::enqueue() the Runnable will be assigned to the thread now", pthread_self());
          enqueuePrivate();
          itc::getLog()->trace(__FILE__, __LINE__, "Thread [%jx] ThreadPool::enqueue() the Runnable has been assigned to the thread", pthread_self());
        }
      }
    }

    const size_t getTaskQueueDepth()
    {
      return mInQueueDepth.load();
    }
    
    void stopPool()
    {
      stopRunning();
      cleanInQueue();
      onShutdown();
    }
    
    ~ThreadPool() noexcept // gcc 4.7.4 compat
    {
      stopPool();
    }

   private:
    itc::sys::mutex mMutex;
    std::atomic<size_t>   mMaxThreads;
    std::atomic<size_t>   mMinThreads;
    std::atomic<bool>     mAutotune;
    std::atomic<float>    mOvercommitRatio;
    std::queue<TaskType>  mTaskQueue;
    std::list<ThreadPTR>  mActiveThreads;
    std::queue<ThreadPTR> mPassiveThreads;
    std::atomic<bool>     doRun;
    std::atomic<size_t>   mInQueueDepth;

    void spawnThreads(size_t n)
    {
      for(size_t i = 0; i < n; i++)
      {
        mPassiveThreads.push(std::make_shared<sys::PThread>());
      }
    }

    void shakePoolsPrivate()
    {
      ThreadListIterator it = mActiveThreads.begin();

      while(it != mActiveThreads.end())
      {
        if(it->get()->getState() == DONE)
        {
          if((getThreadsCount() > mMaxThreads)&&(mTaskQueue.empty()))
          {
            mActiveThreads.erase(it);
          }else
          {
            mPassiveThreads.push(*it);
            mActiveThreads.erase(it);
          }
          it = mActiveThreads.begin();
        }else if(it->get()->getState() == CANCEL)
        {
          mActiveThreads.erase(it);
          it = mActiveThreads.begin();
        }else ++it;
      }

      while(mPassiveThreads.size() > mMaxThreads)
      {
        mPassiveThreads.pop();
      }
    }

    void cleanInQueue()
    {
      ITCSyncLock dosync(mMutex);
      while(!mTaskQueue.empty())
      {
        mTaskQueue.pop();
      }
    }

    void enqueuePrivate()
    {
      if(!mPassiveThreads.empty())
      {
        auto aThread = std::move(mPassiveThreads.front());
        mPassiveThreads.pop();
        if(!mTaskQueue.empty())
        {
          aThread->setRunnable(std::move(mTaskQueue.front()));
          mTaskQueue.pop();
          mInQueueDepth--;
        }
        mActiveThreads.push_back(std::move(aThread));
      }
    }

    void stopRunning()
    {
      doRun = false;
    }

    void onShutdown()
    {
      ITCSyncLock dosync(mMutex);
      while(!mPassiveThreads.empty())
        mPassiveThreads.pop();
      mActiveThreads.clear();
    }
  };
}

#endif    /* _THREADPOOL_H */

