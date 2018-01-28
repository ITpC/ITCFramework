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

namespace itc
{
  /**
   * @brief thread safe blocking queue. The only two operations are available:
   * send and recv. recv will block until message is received.
   */
  template <typename DataType> class tsbqueue
  {
  private:
   std::mutex mMutex;
   itc::sys::Semaphore mEvent;
   std::queue<DataType> mQueue;

  public:
   explicit tsbqueue():mMutex(),mEvent(),mQueue(){};
   tsbqueue(const tsbqueue&)=delete;
   tsbqueue(tsbqueue&)=delete;
   
   void destroy()
   {
     mEvent.destroy();
     SyncLock sync(mMutex);
     while(!mQueue.empty())
       mQueue.pop();
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
    void send(const DataType& ref)
    {
      SyncLock sync(mMutex);
      mQueue.push(ref);
      if(!mEvent.post())
        throw std::system_error(errno,std::system_category(),"Can't increment semaphore");
    }

    const bool tryRecv(DataType& result,const ::timespec& timeout)
    {
      if(mEvent.timedWait(timeout))
      {
        SyncLock sync(mMutex);
        result=mQueue.front();
        mQueue.pop();
        return true;
      }
      return false;
    }
    
    void recv(DataType& result)
    {
      if(!mEvent.wait())
        throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
      else
      {
        SyncLock sync(mMutex);
        
        if(mQueue.empty()) 
          throw std::logic_error("tbsqueue<T>::recv() - already consumed");
        
        result=mQueue.front();
        mQueue.pop();
      }
    }
    
    // it is very slow ...
    // it is better to use const bool size(int&) in the end user code
    const bool empty()
    {
      SyncLock sync(mMutex);
      int value;
      if(this->size(value))
      {
        return value == 0;
      }
      throw std::system_error(errno,std::system_category(),"Can't access semaphore value");
    }
    const bool size(int& value)
    {
      if(!mEvent.getValue(value))
        throw std::system_error(errno,std::system_category(),"Can't access semaphore value");
      return true;
    }
  };
}
#endif	/* __TSBQUEUE_H__ */

