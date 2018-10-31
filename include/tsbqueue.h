/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: tsbqueue.h 3 Май 2015 г. 17:05 pk$
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef __TSBQUEUE_H__
#  define	__TSBQUEUE_H__

#include <queue>
#include <sys/PosixSemaphore.h>
#include <sys/synclock.h>
#include <mutex>
#include <sys/mutex.h>

namespace itc
{
  /**
   * @brief thread safe blocking queue. 
   */
  template <typename DataType, typename MutexType=std::mutex> class tsbqueue
  {
  private:
   MutexType             mMutex;
   itc::sys::Semaphore   mEvent;
   std::queue<DataType>  mQueue;
   std::atomic<size_t>   mQueueDepth;
  public:
   explicit tsbqueue():mMutex(),mEvent(),mQueue(),mQueueDepth{0}{};
   tsbqueue(const tsbqueue&)=delete;
   tsbqueue(tsbqueue&)=delete;
   
   void destroy()
   {
     mEvent.destroy();
     std::lock_guard<MutexType> sync(mMutex);
     while(!mQueue.empty())
       mQueue.pop();
     mQueueDepth=0;
   }
   
   ~tsbqueue()
   {
     destroy();
   }
   /**
    * @brief send message of DataType to the queue.
    * @param ref message to be sent.
    * @return self
    */
    void send(const std::vector<DataType>& ref)
    {
      std::lock_guard<MutexType> sync(mMutex);
      for(size_t i=0;i<ref.size();++i)
      {
        mQueue.push(std::move(ref[i]));
        ++mQueueDepth;
        if(!mEvent.post())
        {
          throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
        }
      }
    }
    
    const bool try_send(const std::vector<DataType>& ref)
    {
      if(mMutex.try_lock())
      {
        for(size_t i=0;i<ref.size();++i)
        {
          mQueue.push(std::move(ref[i]));
          ++mQueueDepth;
          if(!mEvent.post())
          {
            throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
          }
        }
        mMutex.unlock();
        return true;
      }
      return false;
    }
    
   /**
    * @brief send message of DataType to the queue.
    * @param ref message to be sent.
    * @return self
    */
    void send(const DataType& ref)
    {
      std::lock_guard<MutexType> sync(mMutex);
      mQueue.push(std::move(ref));
      ++mQueueDepth;
      if(!mEvent.post())
      {
        throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
      }
    }
    
    const bool tryRecv(DataType& result,const ::timespec& timeout)
    {
      if(mEvent.timed_wait(timeout))
      {
        std::lock_guard<MutexType> sync(mMutex);
        result=std::move(mQueue.front());
        mQueue.pop();
        --mQueueDepth;
        return true;
      }
      return false;
    }
    
    const bool tryRecv(DataType& result)
    {
      if(mEvent.try_wait())
      {
        std::lock_guard<MutexType> sync(mMutex);
        result=std::move(mQueue.front());
        mQueue.pop();
        --mQueueDepth;
        return true;
      }
      return false;
    }
    
    /**
     * @brief receive a message from queue as it is available. This method will
     * block while queue is empty. As far as a semaphore indicates that there 
     * is a new message it attempts to receive it. If there are concurrent 
     * threads waiting on the same queue and it appears that the expected message
     * is already read (which is very unlikely though possible scenario), it 
     * throws an exception.
     * 
     **/
    void recv(DataType& result)
    {
      if(!mEvent.wait())
        throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
      else
      {
        std::lock_guard<MutexType> sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv(T&) - already consumed");
        result=std::move(mQueue.front());
        mQueue.pop();
        --mQueueDepth;
      }
    }
    
    auto recv()
    {
      if(!mEvent.wait())
        throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
      else
      {
        std::lock_guard<MutexType> sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv() - already consumed");

        auto result=std::move(mQueue.front());
        mQueue.pop();
        --mQueueDepth;
        return result;
      }
    }
    
    void recv(std::queue<DataType>& out)
    {
      if(!mEvent.wait())
        throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
      else
      {
        std::lock_guard<MutexType> sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv(std::queue<DataType>&) - already consumed");
        
        while(!mQueue.empty())
        {
          out.push(std::move(mQueue.front()));
          mQueue.pop();
          --mQueueDepth;
          mEvent.try_wait();
        }
      }
    }
    const size_t size() const
    {
      return mQueueDepth;
    }
  };
}
#endif	/* __TSBQUEUE_H__ */

