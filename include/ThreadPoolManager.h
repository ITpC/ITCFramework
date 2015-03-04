/**
 * Copyright (c) 2007-2015, Pavel Kraynyukhov.
 *  
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted under the terms of the General Public License version 2
 * (GPLv2), provided that the above copyright notice and this paragraph and the
 * following two paragraphs and the "LICENSE" file appear in all modified or
 * unmodified copies of the software "AS IS" and without any changes.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 * 
 * 
 * $Id: ThreadPoolManager.h 1 2015-02-28 13:43:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef THREADPOOLMANAGER_H
#define	THREADPOOLMANAGER_H

#include <memory>
#include <stdint.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <ThreadPool.h>
#include <abstract/Runnable.h>
#include <exception>
#include <sched.h>
#include <sys/SemSleep.h>

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
    sys::Mutex    mMutex;
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
      sys::SyncLock synchronize(mMutex);
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
      sys::SyncLock synchronize(mMutex);
      doStart=true;
    }
    
    void enqueueRunnable(abstract::IThreadPool::value_type& ref)
    {
      mThreadPool.get()->enqueue(ref);
    }
    
    
    void execute()
    {
      ::itc::sys::SemSleep mSleep;
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
          sys::SyncLock synchronize(mMutex);
    
          if(mTPStats.tpc<mMinReadyThr)
          {
            if((mTPStats.tc<(mMaxThreads+mOvercommitThreads)) && (mTPStats.tqdp>(mTPStats.tc*1.25)))
            {
                mThreadPool.get()->expand(size_t(log2(mTPStats.maxthreads)));
            }
          }

          if((mTPStats.tpc>mMinReadyThr)&&(mTPStats.tpc>=mTPStats.tac)&&((mTPStats.tqdp*2)<mTPStats.maxthreads))
          {
            mThreadPool.get()->reduce(1);
          }
        }catch (std::exception& e)
        {
          throw e;
        }
        mSleep.usleep(mPurgeTm);
      }
    }
    
    void logStats()
    {
      ::itc::getLog()->info("tc:%ju  pc:%ju  ac:%ju qd:%ju mt:%ju",mTPStats.tc, mTPStats.tpc, mTPStats.tac, mTPStats.tqdp,mTPStats.maxthreads);
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

