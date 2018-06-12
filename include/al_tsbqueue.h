/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: al_tsbqueue.h 3 Created on June 9, 2018, 12:53 PM $
 * 
 * EMail: pavel.kraynyukhov@gmail.com
 * 
 **/

#ifndef AL_TSBQUEUE_H
#define AL_TSBQUEUE_H
#include <queue>
#include <sys/PosixSemaphore.h>
#include <sys/atomic_mutex.h>
#include <sys/synclock.h>

namespace itc
{
  /**
   * @brief thread safe blocking queue. 
   */
  template <typename DataType> class al_tsbqueue
  {
  private:
   itc::sys::AtomicMutex mMutex;
   itc::sys::Semaphore   mEvent;
   std::queue<DataType>  mQueue;

  public:
   explicit al_tsbqueue():mMutex(),mEvent(),mQueue(){};
   al_tsbqueue(const al_tsbqueue&)=delete;
   al_tsbqueue(al_tsbqueue&)=delete;
   
   void destroy()
   {
     mEvent.destroy();
     AtomicLock sync(mMutex);
     while(!mQueue.empty())
       mQueue.pop();
   }
   
   ~al_tsbqueue()
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
      AtomicLock sync(mMutex);
      for(size_t i=0;i<ref.size();++i)
      {
        mQueue.push(ref[i]);
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
      AtomicLock sync(mMutex);
      mQueue.push(ref);
      if(!mEvent.post())
      {
        throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
      }
    }
    
    const bool tryRecv(DataType& result,const ::timespec& timeout)
    {
      if(mEvent.timedWait(timeout))
      {
        AtomicLock sync(mMutex);
        result=mQueue.front();
        mQueue.pop();
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
        AtomicLock sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv(T&) - already consumed");
        result=std::move(mQueue.front());
        mQueue.pop();
        mMutex.unlock();
      }
    }
    
    auto recv()
    {
      if(!mEvent.wait())
        throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
      else
      {
        AtomicLock sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv() - already consumed");

        auto result=std::move(mQueue.front());
        mQueue.pop();
        return result;
      }
    }
    
    const bool size(int& value)
    {
      if(!mEvent.getValue(value))
        throw std::system_error(errno,std::system_category(),"Can't access semaphore value");
      return true;
    }
  };
}


#endif /* AL_TSBQUEUE_H */

