/**
 * Copyright (c) 2007, Pavel Kraynyukhov.
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
  * $Id: ThreadPool.h 22 2010-11-23 12:53:33Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 **/

#include <memory>
#include <Val2Type.h>
#include <bdmap.h>
#include <list>
#include <algorithm>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <abstract/IThreadPool.h>
#include <sys/PThread.h>
#include <TSLog.h>
#include <queue>
#include <Date.h>




#ifndef __THREADPOOL_H__
#    define    __THREADPOOL_H__


namespace itc
{
  /**
   * @brief This class is a container for taks queues and it does not manage
   * the threads. It is capable to create and invoke threads on task enqueu,
   * however after the task is done, the thread perists in mActiveThreads 
   * container and will not be moved to mPassiveThreads queue.
   * You does not need this class without a itc::ThreadPoolManager. So instantiate
   * one, which will create an instance of itc::ThreadPool within itself and
   * will effectively manage the threads in a pool.
   **/
  class ThreadPool : public abstract::IThreadPool
  {
  public:
      typedef ::itc::sys::PThread::TaskType        TaskType;
      typedef std::shared_ptr<sys::PThread>        ThreadPTR;
      typedef std::list<ThreadPTR>::iterator       ThreadListIterator;

      explicit ThreadPool(
          const size_t maxthreads=10, bool autotune=true,float overcommit=1.2
      ):mMutex(),mMaxThreads(maxthreads),mAutotune(autotune),
          mOvercommitRatio(overcommit),mMayRun(true)
      {
          sys::SyncLock   synchronize(mMutex);
          ::itc::getLog()->debug(
              __FILE__,__LINE__,
              "created ThreadPool::ThreadPool(%ju,%u,%f)",
               mMaxThreads,mAutotune,mOvercommitRatio
          );
          spawnThreads(mMaxThreads);
      }

      inline const bool getAutotune()
      {
          return mAutotune;
      }

      inline void setAutotune(const bool& autotune)
      {
          sys::SyncLock   synchronize(mMutex);
          mAutotune=autotune;
      }

      inline const size_t getMaxThreads()
      {
          return mMaxThreads;
      }

      inline const float getOvercommitRatio() const
      {
        return mOvercommitRatio;
      }

      inline const size_t getThreadsCount()
      {
          return mActiveThreads.size()+mPassiveThreads.size();
      }

      inline const size_t getActiveThreadsCount() const
      {
          return mActiveThreads.size();
      }

      inline const size_t getPassiveThreadsCount() const
      {
          return mPassiveThreads.size();
      }

      inline bool mayRun() const
      {
          return mMayRun;
      }

      inline void expand(const size_t& inc)
      {
          sys::SyncLock   synchronize(mMutex);
          if(mayRun())
          {
              mMaxThreads+=inc;
              for(size_t i=0;i<inc;i++)
                  mPassiveThreads.push(std::make_shared<sys::PThread>());
          }
      }

      inline void reduce(const size_t& dec)
      {
          sys::SyncLock   synchronize(mMutex);
          if(mMaxThreads>dec)
          {
              mMaxThreads-=dec;
              ::itc::getLog()->debug(__FILE__,__LINE__,"ThreadPool::reduce() - pool is reduced to %ju",mMaxThreads);
          }
      }

      inline const size_t getFreeThreadsCount()
      {
        sys::SyncLock   synchronize(mMutex);
        size_t ftc=0;
        std::for_each(
          mActiveThreads.begin(),
          mActiveThreads.end(),
          [&ftc](const std::shared_ptr<itc::sys::PThread>& sptr)
          {
           if(sptr.get()->getState() == DONE)
           {
            ftc++;
           }
          }
        );
        return mPassiveThreads.size()+ftc;
      }

      inline void shakePools()
      {
          sys::SyncLock   synchronize(mMutex);

          if(mayRun())
          {
              shakePoolsPrivate();

              if(mPassiveThreads.empty()&&(mTaskQueue.size()>0)&&mAutotune)
              {
                  size_t absMax =(size_t)(mMaxThreads * mOvercommitRatio);

                  size_t max_start = absMax - getThreadsCount();

                  spawnThreads(max_start);
              }
              while((!mPassiveThreads.empty())&&(!mTaskQueue.empty()))
              {
                  enqueuePrivate();
              }
          }
      }

      void enqueue(value_type& ref)
      {
          sys::SyncLock   synchronize(mMutex);

          if(mayRun())
          {
              mTaskQueue.push(ref);
              itc::getLog()->trace(__FILE__,__LINE__,"Thread [%jx] ThreadPool::enqueue() the Runnable is enqueued",pthread_self());
              if(!mPassiveThreads.empty())
              {
                  itc::getLog()->trace(__FILE__,__LINE__,"Thread [%jx] ThreadPool::enqueue() the Runnable will be assigned to the thread now",pthread_self());
                  enqueuePrivate();
                  itc::getLog()->trace(__FILE__,__LINE__,"Thread [%jx] ThreadPool::enqueue() the Runnable has been assigned to the thread",pthread_self());
              }
          }
      }

      const size_t getTaskQueueDepth()
      {
        sys::SyncLock   synchronize(mMutex);
        return mTaskQueue.size();
      }

      ~ThreadPool() noexcept // gcc 4.7.4 compat
      {
          stopRunning();
          cleanInQueue();
          onShutdown();
      }

  private:
      ::itc::sys::Mutex        mMutex;
      size_t                   mMaxThreads;
      bool                     mAutotune;
      float                    mOvercommitRatio;
      std::queue<TaskType>     mTaskQueue;
      std::list<ThreadPTR>     mActiveThreads;
      std::queue<ThreadPTR>    mPassiveThreads;
      bool                     mMayRun;

      inline void spawnThreads(size_t n)
      {
          for(size_t i=0;i<n;i++)
          {
              mPassiveThreads.push(std::make_shared<sys::PThread>());
          }            
      }

      inline void shakePoolsPrivate()
      {
          ThreadListIterator it=mActiveThreads.begin();

          while(it!=mActiveThreads.end())
          {
              if(it->get()->getState() == DONE)
              {
                  if((getThreadsCount()>mMaxThreads)&&(mTaskQueue.empty()))
                  {
                      mActiveThreads.erase(it);
                  }
                  else
                  {
                      mPassiveThreads.push(*it);
                      mActiveThreads.erase(it);
                  }                    
                  it=mActiveThreads.begin();
              }
              else if(it->get()->getState() == CANCEL)
              {
                  mActiveThreads.erase(it);
                  it=mActiveThreads.begin();
              }
              else it++;
          }

          while(mPassiveThreads.size()>mMaxThreads)
          {
            mPassiveThreads.pop();
          }
      }

      inline void cleanInQueue()
      {
          sys::SyncLock   synchronize(mMutex);
          while(!mTaskQueue.empty())
          {
            mTaskQueue.pop();
          }
      }

      inline void enqueuePrivate()
      {
          ThreadPTR aThread=mPassiveThreads.front();
          aThread->setRunnable(mTaskQueue.front());
          mTaskQueue.pop();
          mActiveThreads.push_back(aThread);
          mPassiveThreads.pop();                    
      }

      inline void stopRunning()
      {
          sys::SyncLock   synchronize(mMutex);
          mMayRun=false;
      }
      inline void onShutdown()
      {
        sys::SyncLock   synchronize(mMutex);
        while(!mActiveThreads.empty())
        {
          shakePoolsPrivate();
          sched_yield();
        }
      }
    };
}

#endif    /* _THREADPOOL_H */

